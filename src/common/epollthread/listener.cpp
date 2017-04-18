/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       �ļ�����:  listener.cpp
 *       �ļ�����:  �����࣬��Ҫ�û�����������socket����
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.22 
 *       �޸ļ�¼:  
 ***************************************************************************/

 
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/tcp.h>

#include  "listener.h"
#include  "log.h"



int SockBind (const char *pAddr, unsigned short usPort, unsigned int uiAcceptNum)
{
	struct sockaddr_in inaddr;
	int reuse_addr = 1;
	int iNetfd;

	bzero (&inaddr, sizeof (struct sockaddr_in));
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons (usPort);

	if(inet_pton(AF_INET, pAddr, &inaddr.sin_addr) <= 0)
	{

		EMERG_LOG("invoid listen addr [%s] port[%d]",pAddr, usPort);
		return -1;
	}

	if((iNetfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		EMERG_LOG("create tcp socket error");
		return -1;
	}

	/*���÷������Ķ˿ڿ�����,reuse_addr Ϊ�漴��*/
	setsockopt (iNetfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof (reuse_addr));

	/*���÷������ܹ����Ϸ���Ӧ�������������nagle�㷨*/
	
	setsockopt (iNetfd, SOL_TCP, TCP_NODELAY, &reuse_addr, sizeof (reuse_addr));

	
	reuse_addr = 60;

	/*���ȴ�����ACK�������ڵ�1�����������ݵİ�����ų�ʼ����������*/
	/*reuse_addr�ĵ�λ���룬kernel �� reuse_addr ��֮�ڻ�û���յ����ݣ�����������ѽ��̣�����ֱ�Ӷ�������*/
	setsockopt (iNetfd, SOL_TCP, TCP_DEFER_ACCEPT, &reuse_addr, sizeof (reuse_addr));

	if(bind(iNetfd, (struct sockaddr *)&inaddr, sizeof(struct sockaddr)) == -1)
	{
		EMERG_LOG("bind tcp %s:%u failed, %m", pAddr, usPort);
		close (iNetfd);
		return -1;
	}

	if(listen(iNetfd, uiAcceptNum) == -1)
	{
		EMERG_LOG("listen tcp %s:%u failed", pAddr, usPort);
		close (iNetfd);
		return -1;
	}

	INFO_LOG("listen on tcp %s:%u", pAddr, usPort);
	return iNetfd;
}


CListener::CListener (const char* pAddr, unsigned short usPort, unsigned int uiAcceptNum, CClientManage* pCClientManage)
{

	strncpy(m_szListenIP, pAddr, MAX_IP_ADDR_LEN);
	m_uiHasBind = 0;
	m_usListenPort = usPort;
	m_uiAcceptNum = uiAcceptNum;
	m_pCClientManage = pCClientManage;

}

CListener::~CListener()
{
	return;
}

/*************************************************************
*��������: Bind()
*�������: ��
*���ز���: ��
*��������: ����ָ���˿�
************************************************************/
int CListener::Bind ()
{
	if(m_uiHasBind)
	{
		return SUCCESS_CODE;
	}
	
	if((m_iNetFd = SockBind (m_szListenIP, m_usListenPort, m_uiAcceptNum)) == -1)
	{
		return ERROR_OTHER_FAILED;
	}
	
	m_uiHasBind = 1;
	
	return SUCCESS_CODE;
}

/*************************************************************
*��������: Attach()
*�������: ��
*���ز���: ��
*��������: �������˿�attach��epoll��
************************************************************/
int CListener::Attach(CPollThread * pPollThread)
{
	if(Bind() != 0)
	{
		return -1;
	}
	
	m_pMainThread = pPollThread;

	/*����������Ϣ*/
	EnableInput();
	return AttachPoller(m_pMainThread);
}

/*************************************************************
*��������: InputNotify()
*�������: ��
*���ز���: ��
*��������: ��������˿ڵ�input �¼�
************************************************************/
void CListener::InputNotify (void)
{
	int iNewfd = -1;
	socklen_t peerSize;
	struct sockaddr peer;

	while (true)
	{
		peerSize = sizeof (peer);
		iNewfd = accept (m_iNetFd, &peer, &peerSize);
		if (iNewfd < 0)
		{
			/*���ʧ�ܣ������ԣ�ֱ������������ͨ����Ϣ�¼�����*/
			if ((errno != EINTR) && (errno != EAGAIN  ))
			{
				ERROR_LOG("reason[accept_failed]fd[%u]error[%d]errmsg[%s]", m_iNetFd, errno, strerror(errno));
			}
			break;
		}

		if(m_pCClientManage->ProcessIncoming(iNewfd, &peer, peerSize))
		{
			CRIT_LOG("reason[creat_client_failed]fd[%u]", iNewfd);
			close(iNewfd);
		}

	}
}

