/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称: client_sync.cpp
 *       文件功能: 客户端连接管理类及客户端连接类
 *       文件作者:   
 *       修改时间:  2011.03.21 
 *       修改记录:  
 ***************************************************************************/

#include <stdio.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client_sync.h"
#include "log.h"

#include "dc_process.h"



/*************************************************************************** 
 *       类      名称: CClientManage
 *       类      功能: 客户端连接管理类
 				    1:创建客户端连接类
 				    2:维持链路激活链表
 				    3:维持消息超时链表
 *       文件作者:   
 *       修改时间:  2011.03.21 
 *       修改记录:  
 ***************************************************************************/
 
CClientManage::CClientManage(CPollThread* pMainThread, unsigned int uiKeepAliveTime, unsigned int uiTimeOut, bool bMultiThread)
{
	m_pMainThread = pMainThread;
	m_uiLinkAliveTime = uiKeepAliveTime;
	m_uiMsgAliveTime = uiTimeOut;

	m_pLinkAliveList = NULL;
	m_pMsgAliveList = NULL;
		
	m_pLinkAliveList = m_pMainThread->GetTimerList(m_uiLinkAliveTime);

	m_bMultiThread = bMultiThread;

	/*对于无后端通讯的server ，此处不需要消息超时机制*/
	//m_pMsgAliveList = m_pMainThread->GetTimerList(m_uiLinkAliveTime);
}

CClientManage::~CClientManage()
{
	
}

/*************************************************************
*函数名称: ProcessIncoming()
*输入参数: 略
*返回参数: 略
*函数功能: 监听类调用此函数创建连接类
************************************************************/
int CClientManage::ProcessIncoming(int iNetFd, void *pPeerAddr, int iPeerAddrSize)
{
	/*创建客户端连接类*/
	CClientSync* pCClientSync = new CClientSync(this, iNetFd, pPeerAddr, iPeerAddrSize);
	if (NULL == pCClientSync)
	{
		CRIT_LOG("reason[new_failed]object[CClientSync]");
		return ERROR_SYSTEM_FAILED;
	}

	/*将客户端连接加入epoll 监听*/
	if (pCClientSync->Attach () ) 
	{
		CRIT_LOG("reason[CClientSync Attach failed]");
		delete pCClientSync;
		return -1;
	}

	/*因为监听设置了TCP_DEFER_ACCEPT 消息，所以接收到accept时表示有请求消息到达*/
	/*处理请求消息*/
	pCClientSync->InputNotify();

	
	return SUCCESS_CODE;
}




/*************************************************************************** 
 *       类      名称: CClientSync
 *       类      功能: 客户端连接类
 				    1: 管理与客户端的通讯通道
 				    2: 接收和发送消息
 				    3: 管理通信消息
 *       文件作者:  
 *       修改时间: 2011.03.21 
 *       修改记录:  
 ***************************************************************************/
CClientSync::CClientSync(CClientManage *pCClientManage, int iNetFd, void *pPeerAddr, int iPeerAddrSize):
	CPollerObject (pCClientManage->m_pMainThread, iNetFd)
{
	struct sockaddr_in *bAddr;
	bAddr=(struct sockaddr_in *)(pPeerAddr);
	strncpy(m_szRemoteIPAddr,(char *)inet_ntoa(bAddr->sin_addr) ,MAX_IP_ADDR_LEN);
	m_usRemotePort = bAddr->sin_port;
	stage = IdleState;
	m_pMsgSync = NULL;
	m_pClientManage = pCClientManage;
}

CClientSync::~CClientSync ()
{    
	if(m_pMsgSync) 
	{
		if(stage == ProcReqState)
		{
			/*如果消息状态为处理中，则说明为多线程处理*/
			/*此处仅将消息中的 客户端连接类指针置为空
			当后继消息处理完后返回，检查该客户端类为空，自己删除 */
			m_pMsgSync->m_pCClientSync = NULL;
		}
		else
		{
			/*删除消息*/
			delete m_pMsgSync;
		}
			
	}

}

/*************************************************************
*函数名称: Attach()
*输入参数: 略
*返回参数: -1:挂入epoll 失败，需要调用函数做删除操作
                              0:挂入epoll 成功
*函数功能: 将客户端连接attach 到epoll ，并挂入激活超时链表
************************************************************/
int CClientSync::Attach (void)
{
	/*激活输入事件，并将客户端连接放入epoll 监听*/
	EnableInput();
	if (AttachPoller ())
	{
		return -1;
	}

	/*并将客户端连接挂入链路激活检查链表*/
	AttachTimer (m_pClientManage->GetLinkAliveList());

	stage = IdleState;
	return 0;
}


/*************************************************************
*函数名称: InputNotify()
*输入参数: 略
*返回参数: 略
*函数功能: 处理epoll 输入事件，调用接收消息处理函数
************************************************************/
void CClientSync::InputNotify (void)
{
	
	if (stage==IdleState || stage==RecvReqState) 
	{
		/*客户端处在空闲状态，或者等待接收消息状态*/
		if(RecvRequest () < 0)
		{
			delete this;
			return;
		}
	}
	else
	{
		/*客户端正在处理或者发送消息，此时需要检查是否链路故障消息*/
		if(CheckLinkStatus())
		{
			ERROR_LOG("reason[socket_close]stage[%u]", stage);
			delete this;
			return;
		}
		else
		{
			/*此时不监听接收事件*/
			DisableInput();
		}
	}

	ApplyEvents();
}



 /*************************************************************
*函数名称: RecvRequest()
*输入参数: 略
*返回参数: -1:接收失败,需要调用函数做删除操作
                              0:接收消息成功,需要前端重新ApplyEvents
*函数功能: 接收来自客户端的消息
			    1:如果为单进程，则在此函数中处理完消息后调用返回消息函数
			    2:如果为多进程，则在此函数中将消息发往其它进程
************************************************************/
int CClientSync::RecvRequest ()
{
    /*检查消息是否为空*/
    if(NULL == m_pMsgSync) 
    {
        m_pMsgSync = new CMessageSync(this);
        if (NULL == m_pMsgSync)
        {
            CRIT_LOG("reason[new_failed]object[CMessageSync]");
            return -1;
        }
        /*提供外部初始化消息类*/
        if(m_pMsgSync->Init())
        {
            CRIT_LOG("reason[CMessageSync Init failed]");
            return -1;
        }
    }

    /*有消息时，从激活超时链表中删除*/
    DisableTimer();

    /*接收消息*/
    int iRet = m_pMsgSync->DecodeMsg(m_iNetFd);
    switch(iRet)
    {
        case -1:
            /*链路发生错误,或者消息格式非法,需要删除客户端*/
            stage = IdleState;
            //ERROR_LOG("reason[rcv_msg_fail]fd[%d]",m_iNetFd);
            return -1;
            break;
        case -2:
            /*链路上没有任何消息,将状态置为空闲*/
            stage = IdleState;
            AttachTimer(m_pClientManage->GetLinkAliveList());
            return 0;
            break;
        case 0:
            /*消息未收完，继续接收*/
            stage = RecvReqState;
            m_ulLastAliveTime = GET_TIMESTAMP();
            ERROR_LOG("waiting for recv more msg fd [%d]", m_iNetFd);
            return 0;
            break;
        default:
            /*消息已经收完,开始处理消息*/
            stage = ProcReqState;
            m_ulLastAliveTime = GET_TIMESTAMP();
            break;
    }

    if(NOT_DC_PROTO != m_pMsgSync->m_ucVersion)
    {
        if(CDataProcess::Instance()->Decode(m_pMsgSync->m_szRcvBuf,m_pMsgSync->m_uiRcvLen))
        {
            ERROR_LOG("Decode[fail!!!!!]");
            return -1;
        }

        /*这里进行top排序*/
        CDataProcess::Instance()->SetTopMsg();
        //调用上面的函数后就会生成top排序的结构CDataProcess::Instance()->m_Top;


        /*设置后端bitservice,不管处理是否成功都进行top排序*/
        CDataProcess::Instance()->Process();

        stage = IdleState;
        m_pMsgSync->ClearEnvironment();
        return 0;
    }

    /*消息处理,对于某些情况需要直接关闭连接，方便前端识别*/
    if(m_pMsgSync->ProcessMsg())
    {
        ERROR_LOG("ProcessMsg[fail!!!!!]");
        stage = IdleState;
        m_pMsgSync->ClearEnvironment();
        return -1;
    }

    /*如果消息处理在单线程内完成，则直接返回响应消息*/
    if(!m_pClientManage->m_bMultiThread)
    {
        if(SndResponse())
        {
            ERROR_LOG("SndResponse[fail!!!!!]");
            m_pMsgSync->ClearEnvironment();
            return -1;
        }
        m_pMsgSync->ClearEnvironment();
    }

    return 0;
}


 /*************************************************************
*函数名称: SndResponse()
*输入参数: 略
*返回参数: -1:发送消息失败,需要调用函数做删除操作
                              0:发送消息成功,需要前端重新ApplyEvents
*函数功能: 发送响应消息
			    1:如果为单进程，则在RecvRequest 中调用该函数
			    2:如果为多进程，则在进程消息队列触发时(CMessageSync->m_pCClientSync)调用此函数
			    3:如果第一次消息未发完，则后继由OutputNotify调用此函数
************************************************************/
int CClientSync::SndResponse (void)
{	
	/*调用此函数时，首先将状态置为发送状态*/
	/*如果为多线程,则在消息队列触发调用此函数时,将stage置为SendRepState*/
	stage = SendRepState;
	
	int iRet = m_pMsgSync->SndMsg(m_iNetFd); 

	switch (iRet)
	{
		case -1:
			/*链路发生错误,或者消息格式非法,需要删除客户端*/
			stage = IdleState;
			ERROR_LOG("reason[snd_msg_fail]fd[%d]",m_iNetFd);
			return -1;
			break;
		case 0:
			/*消息未发送完，需要监听输出事件*/
			stage = SendRepState;
			EnableOutput();
			DEBUG_LOG("waiting for snd more msg fd [%d]", m_iNetFd);
			return 0;
			break;
		default:
			/*消息已经发送完成，需要去除监听输出事件*/
			stage = IdleState;
			DisableOutput();
			EnableInput();
			AttachTimer(m_pClientManage->GetLinkAliveList());
			break;
	}
	
	return 0;
}

 /*************************************************************
*函数名称: OutputNotify()
*输入参数: 略
*返回参数: 略
*函数功能: 发送响应消息，由epoll的输出事件触发
************************************************************/
void CClientSync::OutputNotify (void)
{
	if (SendRepState == stage) 
	{
		if(SndResponse())
		{
			/*发送失败，清除相关资源*/
			delete this;
		}	
	} 
	else
	{
		DisableOutput();
		ERROR_LOG("reason[output_illagle]fd[%d]",m_iNetFd);
	}
}

