/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef __CLIENT_SYNC_H__
#define __CLIENT_SYNC_H__

#include "poll_thread.h"
#include "msg_sync.h"


class CClientManage;
class CClientSync;

enum CClientState 
{
	IdleState,
	RecvReqState, //wait for recv request, server side
	SendRepState, //wait for send response, server side
	ProcReqState, // IN processing
};

/*客户端连接管理类*/
class CClientManage
{

public:
	CClientManage (CPollThread* pMainThread, unsigned int uiKeepAliveTime, unsigned int uiTimeOut, bool bMultiThread = false);
	~CClientManage();

	int ProcessIncoming(int iNetFd, void *pPeerAddr, int iPeerAddrSize);
	
	CTimerList *GetLinkAliveList()  { return m_pLinkAliveList; }
	CTimerList *GetMsgAliveList()  { return m_pMsgAliveList; }
	int GetLinkAliveTime()  { return m_uiLinkAliveTime; }
	int GetMsgAliveTime()  { return m_uiMsgAliveTime; }
	

public:
	CPollThread* m_pMainThread;        /*主线程类*/

	unsigned int  m_uiLinkAliveTime;       /*链路激活时间*/
	
	unsigned int  m_uiMsgAliveTime;   /*消息超时时间*/
	
	CTimerList *m_pLinkAliveList;
	
	CTimerList *m_pMsgAliveList;

	bool m_bMultiThread;
	
};




/*客户端连接类*/
class CClientSync : public CPollerObject, private CTimerObject 
{
public:
	
	CClientSync (CClientManage *pCClientManage, int iNetFd, void *pPeerAddr, int iPeerAddrSize);
	virtual ~CClientSync ();

	virtual int Attach (void);
	virtual void InputNotify (void);
	virtual void OutputNotify (void);
	int RecvRequest (void);
	int SndResponse (void);

	
	int SendResult(void);

	void AdjustEvents(void);

public:
	
	CClientManage * m_pClientManage;         /*客户端管理类*/         
	
	CClientState stage;                                  /*链路状态*/         
	
	CMessageSync *m_pMsgSync;                /*消息类*/    

	char m_szRemoteIPAddr[MAX_IP_ADDR_LEN];
	unsigned short m_usRemotePort;

};




#endif

