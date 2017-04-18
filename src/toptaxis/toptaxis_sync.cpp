/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_sync.cpp
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description:    TOP����ϵͳ��������ͬ����ʵ��
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>

#include "strutil.h"

#include "cache_manage.h"
#include "poll_thread.h"
#include "listener.h"
#include "client_sync.h"
#include "log.h"
#include "timestamp.h"

#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "toptaxis_conf.h"
#include "toptaxis_backup.h"
#include "toptaxis_sync.h"
#include "comm_semp.h"

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include<sched.h> 
#include<ctype.h> 
#include <map>

#include "dc_process.h"
#include "send_warn.h"

using namespace std;

/*******************************************************************************
* Description: Notifyͬ����Ĺ��캯������ʼ����Ա����
*       Input:  ��
*      Output: ��
*      Return: ��
*      Others: ��
*******************************************************************************/
CNotifySync::CNotifySync()
{
    m_iTimeOut = 0;
    m_usPort = 0;    
    bzero(m_szIP, MAX_IP_ADDR_LEN);
}
/*******************************************************************************
* Description: Notifyͬ����������������ͷ���Դ
*       Input:  ��
*      Output: ��
*      Return: ��
*      Others: ��
*******************************************************************************/
CNotifySync::~CNotifySync()
{
      m_stTcpClient.CloseSocket();
}
/*******************************************************************************
* Description: Notifyͬ��������notify socket��ʵ��
*         Input:  szIp: notify��ip��ַ
*                usPort: notify�Ķ˿�
*             iTimeOut: socket���ӵĳ�ʱʱ�䵥λ����
*      Output: ��
*      Return: 0 �ɹ�
                  ����: ʧ��
*      Others: ��
*******************************************************************************/
int CNotifySync::Init(char* szIp, unsigned short usPort, int iTimeOut)
{
    int iRet = 0;
    char szErrMessage[MAX_ERROR_INFO_LEN] = {0};
    if((NULL == szIp)||(0 == usPort))
    {
        snprintf(szErrMessage, MAX_ERROR_INFO_LEN, "%s", "szIp or usPort is -1\n");
        return -1;
    }

    iRet = m_stTcpClient.InitServer(szIp, usPort, m_iTimeOut, szErrMessage, MAX_ERROR_INFO_LEN);
    if(iRet)
    {
        //ERROR_LOG("NotifySyncInit[fail]ErrorInfo[%s]", szErrMessage);
        return -1;
    }
    strncpy(m_szIP, szIp, MAX_IP_ADDR_LEN);
    m_usPort = usPort;
    m_iTimeOut = iTimeOut;
    return 0;
}
/*******************************************************************************
* Description: ֪ͨnotify������Ϣͬ��
* @input:
*     uiShmKey: ��Ҫͬ���Ĺ����ڴ�key
*         Others: ��
*******************************************************************************/
int CNotifySync::NotifySyncMsg(unsigned int uiShmKey)
{
    NotiTaxisReq* p_stNotiTaxisReq = NULL;
//    NotiTaxisRsp *p_stNotiTaxisRsp = NULL;
//    unsigned int *p_uiData = NULL;
//    unsigned int i = 0;
//    char *pTemp = NULL;

    int iRet = 0;
    unsigned int ui_SendBufferLen = 0;
    int i_RecvBufferLen = 0;
    Bitmap_Header *pSendBitmapHeader = NULL;
//    Bitmap_Header *pRecvBitmapHeader = NULL;

    /* ������������*/
    pSendBitmapHeader = (Bitmap_Header *)m_szSndBuf;
    /*��ʶ��bitservice�����ݰ�*/
    pSendBitmapHeader->m_cVersion = 0xFF;
    pSendBitmapHeader->m_usFirstLevelCmd = E_FATHER_STORE_TO_NOTIFY;
    pSendBitmapHeader->m_usSecondLevelCmd = E_SERVICE_NOTI;

    p_stNotiTaxisReq = (NotiTaxisReq*)(pSendBitmapHeader->m_szMsgData);

    p_stNotiTaxisReq->m_uiKey = uiShmKey;

    pSendBitmapHeader->m_usMsgLen = TAXIS_NOTIFY_MSG_LEN;

    ui_SendBufferLen = pSendBitmapHeader->m_usMsgLen;

    pSendBitmapHeader->m_usMsgLen = htons(TAXIS_NOTIFY_MSG_BODY_LEN);

    /* �����հ�*/
    iRet = SendAndRecvByLocalConnect(m_szSndBuf, ui_SendBufferLen, m_szRcvBuf, MAX_RESPONSE_MSG_LEN, i_RecvBufferLen);
    if(iRet)
    {
        ERROR_LOG("SendAndRecvByLocalConnect[fail]Error[%d]", iRet);
        return iRet;
    }
    return 0;
}
/*******************************************************************************
* Description: ����TCP�������հ�
* @input:
*       send_buf: ��������
*       send_len: �������ݳ���
*  recv_maxLen: �հ�����󳤶�
* @output:                    
*     recv_buf: �հ�����
*      recv_len:�հ����ݳ���
* return value:  0���ɹ�
*                    others: ʧ��
*         Others: ��
*******************************************************************************/
int CNotifySync::SendAndRecvByLocalConnect(char * send_buf, int send_len, char * recv_buf, int recv_maxLen, 
    int &recv_len)
{
    /* using tcp client connet to send and recv*/
    int iRet = 0;
    char szErrMessage[MAX_ERROR_INFO_LEN] = {0};
    /* send and recv by tcp client socket*/
    iRet = m_stTcpClient.SendAndRcvData(send_buf, send_len, recv_buf, recv_maxLen, szErrMessage, MAX_ERROR_INFO_LEN, 
        recv_len);

    /* fail*/
    if(iRet)
    {
        m_stTcpClient.CloseSocket();
        return iRet;
    }
    return 0;
}
