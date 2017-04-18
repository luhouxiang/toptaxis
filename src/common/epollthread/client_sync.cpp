/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       �ļ�����: client_sync.cpp
 *       �ļ�����: �ͻ������ӹ����༰�ͻ���������
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.21 
 *       �޸ļ�¼:  
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
 *       ��      ����: CClientManage
 *       ��      ����: �ͻ������ӹ�����
 				    1:�����ͻ���������
 				    2:ά����·��������
 				    3:ά����Ϣ��ʱ����
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.21 
 *       �޸ļ�¼:  
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

	/*�����޺��ͨѶ��server ���˴�����Ҫ��Ϣ��ʱ����*/
	//m_pMsgAliveList = m_pMainThread->GetTimerList(m_uiLinkAliveTime);
}

CClientManage::~CClientManage()
{
	
}

/*************************************************************
*��������: ProcessIncoming()
*�������: ��
*���ز���: ��
*��������: ��������ô˺�������������
************************************************************/
int CClientManage::ProcessIncoming(int iNetFd, void *pPeerAddr, int iPeerAddrSize)
{
	/*�����ͻ���������*/
	CClientSync* pCClientSync = new CClientSync(this, iNetFd, pPeerAddr, iPeerAddrSize);
	if (NULL == pCClientSync)
	{
		CRIT_LOG("reason[new_failed]object[CClientSync]");
		return ERROR_SYSTEM_FAILED;
	}

	/*���ͻ������Ӽ���epoll ����*/
	if (pCClientSync->Attach () ) 
	{
		CRIT_LOG("reason[CClientSync Attach failed]");
		delete pCClientSync;
		return -1;
	}

	/*��Ϊ����������TCP_DEFER_ACCEPT ��Ϣ�����Խ��յ�acceptʱ��ʾ��������Ϣ����*/
	/*����������Ϣ*/
	pCClientSync->InputNotify();

	
	return SUCCESS_CODE;
}




/*************************************************************************** 
 *       ��      ����: CClientSync
 *       ��      ����: �ͻ���������
 				    1: ������ͻ��˵�ͨѶͨ��
 				    2: ���պͷ�����Ϣ
 				    3: ����ͨ����Ϣ
 *       �ļ�����:  
 *       �޸�ʱ��: 2011.03.21 
 *       �޸ļ�¼:  
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
			/*�����Ϣ״̬Ϊ�����У���˵��Ϊ���̴߳���*/
			/*�˴�������Ϣ�е� �ͻ���������ָ����Ϊ��
			�������Ϣ������󷵻أ����ÿͻ�����Ϊ�գ��Լ�ɾ�� */
			m_pMsgSync->m_pCClientSync = NULL;
		}
		else
		{
			/*ɾ����Ϣ*/
			delete m_pMsgSync;
		}
			
	}

}

/*************************************************************
*��������: Attach()
*�������: ��
*���ز���: -1:����epoll ʧ�ܣ���Ҫ���ú�����ɾ������
                              0:����epoll �ɹ�
*��������: ���ͻ�������attach ��epoll �������뼤�ʱ����
************************************************************/
int CClientSync::Attach (void)
{
	/*���������¼��������ͻ������ӷ���epoll ����*/
	EnableInput();
	if (AttachPoller ())
	{
		return -1;
	}

	/*�����ͻ������ӹ�����·����������*/
	AttachTimer (m_pClientManage->GetLinkAliveList());

	stage = IdleState;
	return 0;
}


/*************************************************************
*��������: InputNotify()
*�������: ��
*���ز���: ��
*��������: ����epoll �����¼������ý�����Ϣ������
************************************************************/
void CClientSync::InputNotify (void)
{
	
	if (stage==IdleState || stage==RecvReqState) 
	{
		/*�ͻ��˴��ڿ���״̬�����ߵȴ�������Ϣ״̬*/
		if(RecvRequest () < 0)
		{
			delete this;
			return;
		}
	}
	else
	{
		/*�ͻ������ڴ�����߷�����Ϣ����ʱ��Ҫ����Ƿ���·������Ϣ*/
		if(CheckLinkStatus())
		{
			ERROR_LOG("reason[socket_close]stage[%u]", stage);
			delete this;
			return;
		}
		else
		{
			/*��ʱ�����������¼�*/
			DisableInput();
		}
	}

	ApplyEvents();
}



 /*************************************************************
*��������: RecvRequest()
*�������: ��
*���ز���: -1:����ʧ��,��Ҫ���ú�����ɾ������
                              0:������Ϣ�ɹ�,��Ҫǰ������ApplyEvents
*��������: �������Կͻ��˵���Ϣ
			    1:���Ϊ�����̣����ڴ˺����д�������Ϣ����÷�����Ϣ����
			    2:���Ϊ����̣����ڴ˺����н���Ϣ������������
************************************************************/
int CClientSync::RecvRequest ()
{
    /*�����Ϣ�Ƿ�Ϊ��*/
    if(NULL == m_pMsgSync) 
    {
        m_pMsgSync = new CMessageSync(this);
        if (NULL == m_pMsgSync)
        {
            CRIT_LOG("reason[new_failed]object[CMessageSync]");
            return -1;
        }
        /*�ṩ�ⲿ��ʼ����Ϣ��*/
        if(m_pMsgSync->Init())
        {
            CRIT_LOG("reason[CMessageSync Init failed]");
            return -1;
        }
    }

    /*����Ϣʱ���Ӽ��ʱ������ɾ��*/
    DisableTimer();

    /*������Ϣ*/
    int iRet = m_pMsgSync->DecodeMsg(m_iNetFd);
    switch(iRet)
    {
        case -1:
            /*��·��������,������Ϣ��ʽ�Ƿ�,��Ҫɾ���ͻ���*/
            stage = IdleState;
            //ERROR_LOG("reason[rcv_msg_fail]fd[%d]",m_iNetFd);
            return -1;
            break;
        case -2:
            /*��·��û���κ���Ϣ,��״̬��Ϊ����*/
            stage = IdleState;
            AttachTimer(m_pClientManage->GetLinkAliveList());
            return 0;
            break;
        case 0:
            /*��Ϣδ���꣬��������*/
            stage = RecvReqState;
            m_ulLastAliveTime = GET_TIMESTAMP();
            ERROR_LOG("waiting for recv more msg fd [%d]", m_iNetFd);
            return 0;
            break;
        default:
            /*��Ϣ�Ѿ�����,��ʼ������Ϣ*/
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

        /*�������top����*/
        CDataProcess::Instance()->SetTopMsg();
        //��������ĺ�����ͻ�����top����ĽṹCDataProcess::Instance()->m_Top;


        /*���ú��bitservice,���ܴ����Ƿ�ɹ�������top����*/
        CDataProcess::Instance()->Process();

        stage = IdleState;
        m_pMsgSync->ClearEnvironment();
        return 0;
    }

    /*��Ϣ����,����ĳЩ�����Ҫֱ�ӹر����ӣ�����ǰ��ʶ��*/
    if(m_pMsgSync->ProcessMsg())
    {
        ERROR_LOG("ProcessMsg[fail!!!!!]");
        stage = IdleState;
        m_pMsgSync->ClearEnvironment();
        return -1;
    }

    /*�����Ϣ�����ڵ��߳�����ɣ���ֱ�ӷ�����Ӧ��Ϣ*/
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
*��������: SndResponse()
*�������: ��
*���ز���: -1:������Ϣʧ��,��Ҫ���ú�����ɾ������
                              0:������Ϣ�ɹ�,��Ҫǰ������ApplyEvents
*��������: ������Ӧ��Ϣ
			    1:���Ϊ�����̣�����RecvRequest �е��øú���
			    2:���Ϊ����̣����ڽ�����Ϣ���д���ʱ(CMessageSync->m_pCClientSync)���ô˺���
			    3:�����һ����Ϣδ���꣬������OutputNotify���ô˺���
************************************************************/
int CClientSync::SndResponse (void)
{	
	/*���ô˺���ʱ�����Ƚ�״̬��Ϊ����״̬*/
	/*���Ϊ���߳�,������Ϣ���д������ô˺���ʱ,��stage��ΪSendRepState*/
	stage = SendRepState;
	
	int iRet = m_pMsgSync->SndMsg(m_iNetFd); 

	switch (iRet)
	{
		case -1:
			/*��·��������,������Ϣ��ʽ�Ƿ�,��Ҫɾ���ͻ���*/
			stage = IdleState;
			ERROR_LOG("reason[snd_msg_fail]fd[%d]",m_iNetFd);
			return -1;
			break;
		case 0:
			/*��Ϣδ�����꣬��Ҫ��������¼�*/
			stage = SendRepState;
			EnableOutput();
			DEBUG_LOG("waiting for snd more msg fd [%d]", m_iNetFd);
			return 0;
			break;
		default:
			/*��Ϣ�Ѿ�������ɣ���Ҫȥ����������¼�*/
			stage = IdleState;
			DisableOutput();
			EnableInput();
			AttachTimer(m_pClientManage->GetLinkAliveList());
			break;
	}
	
	return 0;
}

 /*************************************************************
*��������: OutputNotify()
*�������: ��
*���ز���: ��
*��������: ������Ӧ��Ϣ����epoll������¼�����
************************************************************/
void CClientSync::OutputNotify (void)
{
	if (SendRepState == stage) 
	{
		if(SndResponse())
		{
			/*����ʧ�ܣ���������Դ*/
			delete this;
		}	
	} 
	else
	{
		DisableOutput();
		ERROR_LOG("reason[output_illagle]fd[%d]",m_iNetFd);
	}
}

