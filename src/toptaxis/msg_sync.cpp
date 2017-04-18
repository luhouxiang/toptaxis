/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称:  msg_sync.cpp
 *       文件功能:  消息管理类
 				      1: 接收请求消息
 				      2: 处理请求消息
 				      3: 发送响应消息
 *       文件作者:   
 *       修改时间:  2011.03.23
 *       修改记录:  
 ***************************************************************************/

 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "msg_sync.h"
#include "log.h"
#include "client_sync.h"
#include "toptaxis_header.h"
#include "toptaxis_handle.h"

extern CCacheManage* g_pCCacheManage;

CMessageSync::CMessageSync(CClientSync* pCClientSync)
{
    m_pCClientSync =  pCClientSync;
    m_szRcvBuf = new char[MAX_REQUEST_MSG_LEN+1];
    //memset(m_szRcvBuf,0,MAX_REQUEST_MSG_LEN+1);
    memset(m_szSndBuf,0,MAX_REQUEST_MSG_LEN+1);
    m_uiRcvLen = 0;
    m_uiSndLen = 0;
    m_uiRequestLen = 0;
    m_uiResponseLen = 0;
    //m_pReqBitmapHeader = NULL;
    //m_pResBitmapHeader = NULL;
    //m_pReqBitmapHeader = (Bitmap_Header*)m_szRcvBuf;
    //m_pResBitmapHeader = (Bitmap_Header*)m_szSndBuf;
}

CMessageSync::~CMessageSync()
{
    if(NULL != m_szRcvBuf)
    {
        delete[] m_szRcvBuf;
        m_szRcvBuf = NULL;
    }
    ClearEnvironment();
}


/*************************************************************
*函数名称: Init()
*输入参数:
*返回参数: 
*函数功能: 提供给消息函数初始化
************************************************************/
 int CMessageSync::Init()
{
    return 0;
}

/*************************************************************
*函数名称: ClearEnvironment()
*输入参数:
*返回参数: 
*函数功能: 每次处理完消息后，清空环境变量
************************************************************/
inline int CMessageSync::ClearEnvironment()
{
    m_uiRcvLen = 0;
    m_uiSndLen = 0;
    m_uiRequestLen = 0;
    m_uiResponseLen = 0;
    bzero(m_szSndBuf, MAX_REQUEST_MSG_LEN);
    if (NULL != m_szRcvBuf)
    {
        bzero(m_szRcvBuf, MAX_REQUEST_MSG_LEN);
    }
    return 0;
}

int CMessageSync::RecvData(char* szRspBuf, int i_fd, unsigned int iRspMaxLen, unsigned int iNeedLen)
{
    if(NULL == szRspBuf)
    {
        ERROR_LOG("%s", "[SendAndRcvData] Input parameter [szRspBuf] is NULL");
        return -1;
    }

    unsigned int iMaxTime = 0;
    unsigned int iCnt = 0;
    int iReadCount = 0, iReadTotal = 0;
    iCnt = iNeedLen;
    char* pStrTmp = NULL;

    pStrTmp = szRspBuf;

    while ( iCnt > 0 )
    {
        /*如果连续2000次收包仍为收满，则直接返回错误*/
        iMaxTime++;

        if(iMaxTime > MAXTIMES_TAXIS_RECV_TRY)
        {
            ERROR_LOG("receive data exceed max times fd[%d]iCnt[%u]iNeedLen[%u]data[%s]", i_fd, iCnt, iNeedLen, pStrTmp);
            iReadTotal = iNeedLen -iCnt;
            if(iReadTotal > 0)
            {
                return iReadTotal;
            }
            return -1;
        }


        iReadCount = read( i_fd, szRspBuf, iCnt );
        if ( iReadCount < 0 )
        {
            if ((EINTR== errno)||(EAGAIN== errno))
            {
                continue; 
            }
            ERROR_LOG("receive data error 1, fd[%d]errno[%d] info[%s]",  i_fd, errno, strerror(errno) );
            return -1;
        }

        if ( iReadCount == 0 )
        {
            return -1;
        }

        szRspBuf += iReadCount;
        iCnt -= iReadCount;
    }
    return 0;
}

/*************************************************************
*函数名称: DecodeMsg()
*输入参数: iNetFd : 连接的fd
*返回参数: 0: 未接收完消息  ；>0: 已接收完消息；
           -1: socket 错误    ；<-1:收到消息长度为0，继续收消息
*函数功能: 接收请求消息，并解析
************************************************************/
int CMessageSync::DecodeMsg(int iNetFd)
{
    int iRet = 0;
    ProHdr* pMsgHead = NULL;
    /*当前接收到的消息长度大于缓冲区，覩两种可能*/
    /*1: 请求消息超大*/
    /*2: 消息非法导致乱序*/
    /*对于上面两种情况的处理均是直接断链*/
    if(m_uiRcvLen >= MAX_REQUEST_MSG_LEN)
    {
        ERROR_LOG("reason[rcv_msg_long]req_len[%u]", m_uiRcvLen);
        return -1;
    }
    
    /*上一次的消息未接受完整*/
    if(m_uiRcvLen > 0)
    {
        iRet = RecvData(m_szRcvBuf+m_uiRcvLen, iNetFd, MAX_REQUEST_MSG_LEN - m_uiRcvLen, m_uiRequestLen-m_uiRcvLen);
        if(iRet != 0)
        {
            /**/
            ERROR_LOG("Data can not recv again!!!");	
            ClearEnvironment();
            return -1;
        }
        else
        {
            pMsgHead = (ProHdr*)m_szRcvBuf;
            m_uiRcvLen = m_uiRequestLen;
            DEBUG_LOG("fd[%d]data[%s]", iNetFd, m_szRcvBuf+LENGTH_OF_PREHEAD_TAXIS);
            m_ucVersion=pMsgHead->ucVersion;
            return m_uiRcvLen;
        }
    }
    /*先接收3个字节的头部*/
    iRet = RecvData(m_szRcvBuf, iNetFd, MAX_REQUEST_MSG_LEN, LENGTH_OF_PREHEAD_TAXIS);
    if(iRet != 0)
    {
        if(iRet > 0)
        {
            m_uiRcvLen = iRet;
            ERROR_LOG("Recv head length is not enough[%d]", iRet);	
            return 0;
        }
        ClearEnvironment();
        return -1;
    }


    /*获取消息长度*/
    pMsgHead = (ProHdr*)m_szRcvBuf;
    m_uiRequestLen = ntohs(pMsgHead->usLen) + sizeof(ProHdr);
    DEBUG_LOG("recv_req_len[%u]fd[%d]", m_uiRequestLen, iNetFd);

    if(MAX_REQUEST_MSG_LEN < m_uiRequestLen)
    {
        ERROR_LOG("[SendAndRcvData] szRspBuf is too short! bufLen[%u]<msglen[%u]", MAX_REQUEST_MSG_LEN, m_uiRequestLen);
        ClearEnvironment();
        return -1;
    }


    /*在接收uiMsgLen 字节的消息体*/
    iRet = RecvData(m_szRcvBuf+LENGTH_OF_PREHEAD_TAXIS, iNetFd, MAX_REQUEST_MSG_LEN, 
        m_uiRequestLen - LENGTH_OF_PREHEAD_TAXIS);
    if(iRet != 0)
    {
        //ClearEnvironment();
        if(iRet > 0)
        {
            m_uiRcvLen = iRet + LENGTH_OF_PREHEAD_TAXIS;
            return 0;
        }
        ClearEnvironment();
        return -1;
    }
    m_uiRcvLen = m_uiRequestLen;
    DEBUG_LOG("fd[%d]data[%s]", iNetFd, m_szRcvBuf+LENGTH_OF_PREHEAD_TAXIS);
    /****/
    m_ucVersion=pMsgHead->ucVersion;

    return m_uiRcvLen;
}


/*************************************************************
*函数名称: EncodeMsg()
*输入参数: 略
*返回参数: 略
*函数功能: 组装返回消息,被ProcessMsg 调用
************************************************************/
int  CMessageSync::EncodeMsg()
{
    return 0;
}


/*************************************************************
*函数名称: ProcessMsg()
*输入参数: 略
*返回参数: 0: 返回响应消息
          -1: 不返回响应消息，直接断链		 
*函数功能: 处理请求消息
************************************************************/
int CMessageSync::ProcessMsg()
{
    Bitmap_Header*  pReqBitmapHeader = (Bitmap_Header*)m_szRcvBuf;;
    Bitmap_Header*  pResBitmapHeader =  (Bitmap_Header*)m_szSndBuf;

    char* pReqBody = (char *)pReqBitmapHeader->m_szMsgData;
    char* pResBody = (char *)pResBitmapHeader->m_szMsgData;

    int iRet;
    unsigned short usBID;
    short sResultCode;

    uint64_t ulStartTime, ulEndTime;

    if(FATHER_TRANSFER_TO_STORE == pReqBitmapHeader->m_usFirstLevelCmd)
    {
        ulStartTime =  GET_TIMESTAMP();
        /*外部系统  到toptaxis 的处理命令*/
        switch(pReqBitmapHeader->m_usSecondLevelCmd)
        {
            case SERVICE_QUERY:
                /*查询类命令*/
                CRIT_LOG("recv[SERVICE_QUERY]");
                iRet = ProcessQryData(pReqBody, pResBody, m_uiResponseLen);
                break;
            case SERVICE_REPLACE:
                /*流水同步类命令*/
                CRIT_LOG("recv[SERVICE_REPLACE]");
                iRet = m_pCCacheManage->ProcessReplaceData(pReqBody, pResBody, &m_uiResponseLen);
                break;
             case SERVICE_DELETE:
                 CRIT_LOG("recv[SERVICE_DELETE]");
                 iRet = m_pCCacheManage->ProcessDelData(pReqBody, pResBody, &m_uiResponseLen);
                 break;
             case SERVICE_RELOAD:
                /*通知Reload配置文件*/
                CRIT_LOG("recv[SERVICE_RELOAD]");
                iRet = ScanRuleSysConf();
                break;
                
            default:
                /*无效二级命令字，返回消息非法*/
                *(short *)pResBody = ERROR_REQUEST_MSG_ILLEGAL;
                m_uiResponseLen = BITMAP_HEADER_LEN + sizeof(short);
                CRIT_LOG("reason[sec_cmd_illegal]cmd[%u] ", pReqBitmapHeader->m_usSecondLevelCmd);
                iRet = SUCCESS_CODE;
                break;
        }

        usBID =  *(unsigned short *)pReqBody;
        sResultCode =  *(short *)pResBody;
        ulEndTime =  GET_TIMESTAMP() - ulStartTime;
        /*对于数据操作类命令，因为涉及前端的自动识别，所以需要判断返回值*/
        if(iRet)
        {
            return -1;
        }

    }
    else
    {
        /*无效一级命令字，返回消息非法*/
        *(short *)pResBody = ERROR_REQUEST_MSG_ILLEGAL;
        m_uiResponseLen = BITMAP_HEADER_LEN + sizeof(short);

        CRIT_LOG("reason[first_cmd_illegal]cmd[%u]", pReqBitmapHeader->m_usFirstLevelCmd);
    }

    /*除了消息长度，请求消息的消息头和响应消息的消息头一致*/
    memcpy(pResBitmapHeader, pReqBitmapHeader, BITMAP_HEADER_LEN);
    pResBitmapHeader->m_usMsgLen = htons(m_uiResponseLen - LENGTH_OF_PREHEAD_TAXIS);

    return 0;
}



/*************************************************************
*函数名称: SndMsg()
*输入参数: iNetFd : 连接的fd
*返回参数: 0: 未发送完消息  ；>0: 已发送完消息；
			    -1: socket 错误  
*函数功能: 发送响应消息
************************************************************/
int CMessageSync::SndMsg(int iNetFd)
{
    if(m_uiResponseLen <= m_uiSndLen)
    {
        ClearEnvironment();
        return 1;
    }

    /*发送响应消息*/
    int uiRet = send(iNetFd, m_szSndBuf+m_uiSndLen, m_uiResponseLen-m_uiSndLen, 0);

    if(uiRet < 0)
    {
        if((EINTR == errno)|| (EAGAIN == errno))
        {
            /*消息未发送完成，继续发送消息*/
            return 0;
        }
        /*socket 错误，关闭连接*/
        ERROR_LOG("reason[socket_error]fd[%d]error[%d]errmsg[%s]" ,iNetFd, errno, strerror(errno));
        return -1;
    }

    m_uiSndLen +=uiRet;

    if(m_uiResponseLen > m_uiSndLen)
    {
        /*消息未发送完成，继续发送消息*/
        return 0;
    }

    /*消息已经发送完成，需要清空消息*/
    ClearEnvironment();

    return 1;
}





