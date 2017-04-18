/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称:  tcp_client.cpp
 *       文件功能:  tcp客户端管理类，用于接收和发送bitmap二进制消息
 *       文件作者:   
 *       修改时间:  2011.04.25 
 *       修改记录:  
 ***************************************************************************/

#include "proto_head.h"
#include "toptaxis_header.h"
#include "tcp_client.h"

CTaxisTCPClientSocket::CTaxisTCPClientSocket()
{
    memset(m_szIP, 0, sizeof(m_szIP));
    m_usPort = 0;
    /*默认值是1秒*/
    m_iTimeOut = 1000;
    m_iBlockMode = 1;
    m_iSock = -1;

    return;
}

CTaxisTCPClientSocket::~CTaxisTCPClientSocket()
{
    if(m_iSock < 0)
    {
        m_iSock = -1;
        return;
    }

    close(m_iSock);

    return;
}

/*************************************************************
*函数名称: InitServer()
*输入参数: szIP         --- 对端ip 
           usPort        --- 对端port
           iTimeOut       ---  超时时间
           szErrorMsg    ---  失败描述
           iMaxErrorLen   ---  失败buf 的最大长度
*返回参数:
           LEVEL_RETURN_SUCCESS   --- 成功 
           LEVEL_RETURN_FAILED       --- szIP 为空
           LEVEL_SYSTEM_ERROR       --- szErrorMsg 为空
*函数功能: 初始化socket, 定义 该类后，只需要初始化一次即可
************************************************************/
int CTaxisTCPClientSocket::InitServer(const char* szIP, unsigned short usPort, unsigned int iTimeOut, 
    char* szErrorMsg, unsigned int iMaxErrorLen)
{
    if(NULL == szErrorMsg)
    {
        fprintf(stderr, "szErrorMsg NULL\n"); //test
        return LEVEL_SYSTEM_ERROR;
    }

    memset(szErrorMsg, 0, iMaxErrorLen);

    if(NULL == szIP)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "[InitServersz] Input parameter 1 [IP] is NULL");
        return LEVEL_RETURN_FAILED;
    }

    strncpy(m_szIP, szIP, MAX_LEVEL_IP_LEN);

    m_usPort =  usPort;

    m_iTimeOut =  iTimeOut;

    int iRet = CreateAndConnect(szErrorMsg, iMaxErrorLen);
    if(iRet != LEVEL_RETURN_SUCCESS)
    {
        return iRet;
    }

    return LEVEL_RETURN_SUCCESS;

}

/*************************************************************
*函数名称: SendAndRcvData()
*输入参数: szReqBuf      --- 请求报文，结尾字符为'\n'
                 iReqLen --- 请求报文长度
                szRspBuf --- 响应报文，结尾字符为'\n'
              iRspMaxLen --- 响应buf 的最大长度
             szErrorMsg  --- 失败描述
           iMaxErrorLen  --- 失败buf 的最大长度
*返回参数:
         LEVEL_RETURN_SUCCESS   --- 成功    
         LEVEL_SYSTEM_ERROR     --- szErrorMsg 为空
         LEVEL_RETURN_FAILED    --- szIP 为空
*函数功能: 发送请求，并接收响应
************************************************************/
int CTaxisTCPClientSocket::SendAndRcvData(char* szReqBuf, unsigned int iReqLen, char* szRspBuf, unsigned int iRspMaxLen, char* szErrorMsg, unsigned int iMaxErrorLen, int& iRecvLen)
{
    if(NULL == szErrorMsg)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    /*如果socket 小于0，则重连一次，失败，则直接返回*/
    if(m_iSock < 0)
    {
        if(Reconnect(szErrorMsg,  iMaxErrorLen))
        {
            return LEVEL_RETURN_FAILED;	
        }
    }


    /*重新设置一下超时时间*/
    struct timeval stTimeVal;
    /*毫秒转换成秒*/
    stTimeVal.tv_sec  = m_iTimeOut/1000;
    /*毫秒转换成微秒*/
    stTimeVal.tv_usec = (m_iTimeOut%1000)*1000;
    setsockopt( m_iSock,SOL_SOCKET,SO_RCVTIMEO,&stTimeVal,sizeof(stTimeVal) );
    setsockopt( m_iSock,SOL_SOCKET,SO_SNDTIMEO,&stTimeVal,sizeof(stTimeVal) );



    if((NULL == szReqBuf) || (NULL == szRspBuf))
    {
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "[SendAndRcvData] Input parameter [szReqBuf/szRspBuf] is NULL");
        return LEVEL_RETURN_FAILED;
    }
    //memset(szErrorMsg, 0, iMaxErrorLen);
    //memset(szRspBuf, 0, iRspMaxLen);

    int iRet;

    iRet = SendData(szReqBuf, iReqLen, szErrorMsg, iMaxErrorLen);
    if(iRet != LEVEL_RETURN_SUCCESS)
    {
        return iRet;
    }

    unsigned int uiMsgLen = 0;


    if(iRspMaxLen < LENGTH_OF_PREHEAD_TAXIS)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "[SendAndRcvData] szRspBuf is too short! bufLen[%u]<[%u]", iRspMaxLen, LENGTH_OF_PREHEAD_TAXIS);
        return LEVEL_RETURN_FAILED;
    }

    /*先接收3个字节的头部*/
    iRet = RecvData(szRspBuf, iRspMaxLen, LENGTH_OF_PREHEAD_TAXIS, szErrorMsg, iMaxErrorLen);
    if(iRet != LEVEL_RETURN_SUCCESS)
    {
        return iRet;
    }


    /*获取消息长度*/

    ProHdr* pMsgHead = (ProHdr*)szRspBuf;
    uiMsgLen = ntohs(pMsgHead->usLen) + sizeof(ProHdr);


    if(iRspMaxLen < uiMsgLen)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "[SendAndRcvData] szRspBuf is too short! bufLen[%u]<msglen[%u]", iRspMaxLen, uiMsgLen);
        return LEVEL_RETURN_FAILED;
    }


    /*在接收uiMsgLen 字节的消息体*/
    iRet = RecvData(szRspBuf+LENGTH_OF_PREHEAD_TAXIS, iRspMaxLen, uiMsgLen - LENGTH_OF_PREHEAD_TAXIS, szErrorMsg, iMaxErrorLen);
    if(iRet != LEVEL_RETURN_SUCCESS)
    {
        return iRet;
    }
    iRecvLen = (int)uiMsgLen;
    return LEVEL_RETURN_SUCCESS;
}

int CTaxisTCPClientSocket::IsConnected( fd_set *rdSet, fd_set *wrSet, fd_set *exSet )
{
    int iError = 0;
    int iLen = sizeof( iError);

    if (getsockopt( m_iSock, SOL_SOCKET, SO_ERROR, &iError, (socklen_t*)&iLen ) < 0 )
    {
        return LEVEL_RETURN_FAILED;
    }

    return iError;
}


int CTaxisTCPClientSocket::CreateAndConnect(char* szErrorMsg, int iMaxErrorLen)
{
    if(NULL == szErrorMsg)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    //创建Socket并建立连接
    m_iSock = socket( AF_INET, SOCK_STREAM, 0 );
    if( m_iSock < 0 )
    {
        m_iSock = -1;
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "create socket error");
        return LEVEL_RETURN_FAILED;
    }

    /*设置读写超时时间*/
    struct timeval stTimeVal;
    /*毫秒转换成秒*/
    stTimeVal.tv_sec  = m_iTimeOut/1000;
    /*毫秒转换成微秒*/
    stTimeVal.tv_usec = (m_iTimeOut%1000)*1000;
    setsockopt( m_iSock,SOL_SOCKET,SO_RCVTIMEO,&stTimeVal,sizeof(stTimeVal) );
    setsockopt( m_iSock,SOL_SOCKET,SO_SNDTIMEO,&stTimeVal,sizeof(stTimeVal) );

    /*设置socket reuse*/
    bool bReuseaddr=true;
    setsockopt(m_iSock,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bool));

   
    /* Get the mode of socket */ 
    int iFlags, iRet;
    if ((iFlags = fcntl(m_iSock, F_GETFL, 0)) < 0) 
    {
        snprintf(szErrorMsg, iMaxErrorLen, "get socket mode failed iSock[%d]", m_iSock);
        CloseSocket();
        return LEVEL_RETURN_FAILED;
    } 

    /* Set socket to nonblocking */ 
    if ( fcntl( m_iSock, F_SETFL, iFlags | O_NONBLOCK )< 0 )
    {
        CloseSocket();
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "set socket mode failed  1, fcntl  failed");
        return LEVEL_RETURN_FAILED;
    }


    /*连接对端server */
    struct sockaddr_in stPeer;
    memset (&stPeer, 0x0, sizeof (stPeer));
    stPeer.sin_family = AF_INET;
    stPeer.sin_port = htons(m_usPort);
    inet_aton(m_szIP, &(stPeer.sin_addr));	
    iRet = connect( m_iSock, (struct sockaddr *)&stPeer, sizeof( stPeer ));

    /*对于非阻塞socket，如果连接没有完成，则返回EINPROGRESS*/
    if ((errno != EINPROGRESS)&&(iRet != 0))
    {
        CloseSocket();
        snprintf(szErrorMsg, iMaxErrorLen, "connect failed, errno[%d] info[%s]",  errno,strerror(errno) );
        return LEVEL_RETURN_FAILED;
    } 
    if (0 == iRet )
    {
        /*重新将socket 设置为 阻塞*/
        if (fcntl(m_iSock, F_SETFL, iFlags & (~O_NONBLOCK)) < 0) 
        { 
            snprintf(szErrorMsg, iMaxErrorLen, "%s", "set socket mode failed  2, fcntl  failed");
            CloseSocket();
            return LEVEL_RETURN_FAILED;
        } 
        return LEVEL_RETURN_SUCCESS;
    }

    /*select */
    fd_set rdEvents;
    fd_set wrEvents;
    fd_set exEvents;

    FD_ZERO( &rdEvents );
    FD_ZERO( &wrEvents );
    FD_ZERO( &exEvents );
    FD_SET( m_iSock, &rdEvents );
    FD_SET( m_iSock, &wrEvents );
    FD_SET( m_iSock, &exEvents );

    /*毫秒转换成秒*/
    stTimeVal.tv_sec  = m_iTimeOut/1000;
    /*毫秒转换成微秒*/
    stTimeVal.tv_usec = (m_iTimeOut%1000)*1000;
    
    iRet = select( m_iSock + 1,NULL, &wrEvents, NULL, &stTimeVal );
    if (iRet < 0 )
    {
        CloseSocket();
        snprintf(szErrorMsg, iMaxErrorLen, "connect(doing select) failed, errno[%d] info[%s]",  errno,strerror(errno) );
        return LEVEL_RETURN_FAILED;
    }
    else if (0 == iRet)
    {
        CloseSocket();
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "connect(doing select) timed out");
        return LEVEL_RETURN_FAILED;
    }
    else if (IsConnected(&rdEvents, &wrEvents, &exEvents ) )
    {
        CloseSocket();
        snprintf(szErrorMsg, iMaxErrorLen, "connect(invoid select) , errno[%d] info[%s]",  errno,strerror(errno) );
        return LEVEL_RETURN_FAILED;
    }



    /*重新将socket 设置为 阻塞*/
    if (fcntl(m_iSock, F_SETFL, iFlags & (~O_NONBLOCK)) < 0) 
    { 
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "set socket mode failed  3, fcntl  failed");
        CloseSocket();
        return LEVEL_RETURN_FAILED;
    } 

    return LEVEL_RETURN_SUCCESS;
}

int CTaxisTCPClientSocket::Reconnect(char* szErrorMsg, unsigned int iMaxErrorLen)
{
    if(NULL == szErrorMsg)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    memset(szErrorMsg, 0, iMaxErrorLen);
    CloseSocket();

    return CreateAndConnect(szErrorMsg, iMaxErrorLen);

}

int CTaxisTCPClientSocket::JudgeEnd(char* szRspBuf, unsigned int iMsgLen)
{
    if(NULL == szRspBuf)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    unsigned int iLoop;
    for(iLoop = 0; iLoop < iMsgLen; iLoop++)
    {
        if('\n' == szRspBuf[iLoop])
        {
            return LEVEL_RETURN_SUCCESS;
        }
    }

    return LEVEL_RETURN_FAILED;
}
int CTaxisTCPClientSocket::RecvData(char* szRspBuf, unsigned int iRspMaxLen, unsigned int iNeedLen, char*  szErrorMsg, unsigned int  iMaxErrorLen)
{
    if(NULL == szErrorMsg)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    if(NULL == szRspBuf)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "[SendAndRcvData] Input parameter [szRspBuf] is NULL");
        return LEVEL_RETURN_FAILED;
    }

    unsigned int iMaxTime = 0;
    unsigned int iCnt;
    int iReadCount;
    iCnt = iNeedLen;


    while ( iCnt > 0 )
    {
        /*如果连续2000次收包仍为收满，则直接返回错误*/
        iMaxTime++;

        if(iMaxTime > MAXTIMES_TAXIS_RECV_TRY)
        {
            snprintf(szErrorMsg, iMaxErrorLen, "receive data exceed max times");
            return LEVEL_RETURN_FAILED;
        }


        iReadCount = read( m_iSock, szRspBuf, iCnt );
        if(iReadCount < 0 )
        {
            if (EINTR== errno)
            {
                continue; 
            }
            snprintf(szErrorMsg, iMaxErrorLen, "receive data error 1, errno[%d] info[%s]",  errno, strerror(errno) );
            return LEVEL_RETURN_FAILED;
        }

        if(iReadCount == 0 )
        {
            snprintf(szErrorMsg, iMaxErrorLen, "receive data error scoket close, errno[%d] info[%s]",  errno, strerror(errno) );
            return LEVEL_RETURN_FAILED;
        }

        szRspBuf += iReadCount;
        iCnt -= iReadCount;
    }
    return LEVEL_RETURN_SUCCESS;
}

int CTaxisTCPClientSocket::SendData(char* szReqBuf,unsigned int iReqLen,char* szErrorMsg,unsigned int iMaxErrorLen)
{
    if(NULL == szErrorMsg)
    {
        return LEVEL_SYSTEM_ERROR;
    }

    if(NULL == szReqBuf)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "%s", "[SendAndRcvData] Input parameter [szReqBuf] is NULL");
        return LEVEL_RETURN_FAILED;
    }

    if( iReqLen <= 0)
    {
        snprintf(szErrorMsg, iMaxErrorLen, "send msg len [%d]", iReqLen);
        return 0;
    }

    char* pTemp = szReqBuf;	
    int iThisSend  = 0;
    unsigned int iHasSend=0;

    while(iHasSend < iReqLen)
    {
        do
        {
            iThisSend = send(m_iSock, pTemp, iReqLen-iHasSend, 0);
        }while((iThisSend<0) && (errno==EINTR));


        if(iThisSend < 0)
        {
            snprintf(szErrorMsg, iMaxErrorLen, "send data error 3, errno[%d] info[%s]",  errno,strerror(errno) );
            return LEVEL_RETURN_FAILED;
        }

        iHasSend += iThisSend;
        pTemp += iThisSend;
    }

    return LEVEL_RETURN_SUCCESS;
}

int CTaxisTCPClientSocket::CloseSocket()
{
    if(m_iSock < 0)
    {
        m_iSock = -1;
        return LEVEL_RETURN_SUCCESS;
    }

    close(m_iSock);
    m_iSock = -1;
    return LEVEL_RETURN_SUCCESS;
}

