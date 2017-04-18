/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef __LISTENER_HEADER_H__
#define __LISTENER_HEADER_H__

#include "poll_thread.h"
#include "client_sync.h"

int SockBind (const char *pAddr, unsigned short  usPort, unsigned int uiAcceptNum);


class CListener : public CPollerObject 
{
public:

	CListener (const char* pAddr, unsigned short usPort, unsigned int uiAcceptNum, CClientManage* pCClientManage);
	~CListener();
	int Bind();
	virtual void InputNotify(void);
	virtual int Attach (CPollThread * pPollThread);
	int GetFD(void) { return m_iNetFd; }

private:
	CClientManage* m_pCClientManage;    /*客户端连接管理类*/
	CPollThread *m_pMainThread;          /*主线程类*/
	
	unsigned int m_uiAcceptNum;           /*accept 同时支持的数量*/
	unsigned int m_uiHasBind;                /*是否已经开启监听*/
	unsigned short  m_usListenPort;
	char m_szListenIP[MAX_IP_ADDR_LEN];

}; 
#endif
