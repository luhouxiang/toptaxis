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
*      Description:    TOP排序系统排序数据同步的实现
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
* Description: Notify同步类的构造函数，初始化成员变量
*       Input:  无
*      Output: 无
*      Return: 无
*      Others: 无
*******************************************************************************/
CNotifySync::CNotifySync()
{
    m_iTimeOut = 0;
    m_usPort = 0;    
    bzero(m_szIP, MAX_IP_ADDR_LEN);
}
/*******************************************************************************
* Description: Notify同步类的析构函数，释放资源
*       Input:  无
*      Output: 无
*      Return: 无
*      Others: 无
*******************************************************************************/
CNotifySync::~CNotifySync()
{
      m_stTcpClient.CloseSocket();
}
/*******************************************************************************
* Description: Notify同步类连接notify socket的实现
*         Input:  szIp: notify的ip地址
*                usPort: notify的端口
*             iTimeOut: socket连接的超时时间单位毫秒
*      Output: 无
*      Return: 0 成功
                  其它: 失败
*      Others: 无
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
* Description: 通知notify进行消息同步
* @input:
*     uiShmKey: 需要同步的共享内存key
*         Others: 无
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

    /* 构建请求数据*/
    pSendBitmapHeader = (Bitmap_Header *)m_szSndBuf;
    /*标识是bitservice的数据包*/
    pSendBitmapHeader->m_cVersion = 0xFF;
    pSendBitmapHeader->m_usFirstLevelCmd = E_FATHER_STORE_TO_NOTIFY;
    pSendBitmapHeader->m_usSecondLevelCmd = E_SERVICE_NOTI;

    p_stNotiTaxisReq = (NotiTaxisReq*)(pSendBitmapHeader->m_szMsgData);

    p_stNotiTaxisReq->m_uiKey = uiShmKey;

    pSendBitmapHeader->m_usMsgLen = TAXIS_NOTIFY_MSG_LEN;

    ui_SendBufferLen = pSendBitmapHeader->m_usMsgLen;

    pSendBitmapHeader->m_usMsgLen = htons(TAXIS_NOTIFY_MSG_BODY_LEN);

    /* 发包收包*/
    iRet = SendAndRecvByLocalConnect(m_szSndBuf, ui_SendBufferLen, m_szRcvBuf, MAX_RESPONSE_MSG_LEN, i_RecvBufferLen);
    if(iRet)
    {
        ERROR_LOG("SendAndRecvByLocalConnect[fail]Error[%d]", iRet);
        return iRet;
    }
    return 0;
}
/*******************************************************************************
* Description: 网络TCP发包并收包
* @input:
*       send_buf: 发包数据
*       send_len: 发包数据长度
*  recv_maxLen: 收包的最大长度
* @output:                    
*     recv_buf: 收包数据
*      recv_len:收包数据长度
* return value:  0：成功
*                    others: 失败
*         Others: 无
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
