/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _MSG_SYNC_H
#define _MSG_SYNC_H


#include "cache_manage.h"
#include "proto_head.h"

#define FIELD_MSGLEN_LEN 3 /*bitmap ��Ϣ��ʶ�����ֶεĴ�С*/

const unsigned char  NOT_DC_PROTO=0XFF;


class CClientSync;
class CMessageSync
{
public:

    CMessageSync(CClientSync *pCClientMange);
    ~CMessageSync ();
     int Init();
    int DecodeMsg(int iNetFd);
    int ProcessMsg();
    int EncodeMsg();
    int SndMsg(int iNetFd);
    int ClearEnvironment();
    int RecvData(char* szRspBuf, int i_fd, unsigned int iRspMaxLen, unsigned int iNeedLen);
public:
    CClientSync* m_pCClientSync;        /*�ͻ���������*/

    char* m_szRcvBuf;  /*����buf*/

    char m_szSndBuf[MAX_REQUEST_MSG_LEN+1];  /*��Ӧbuf*/

    unsigned int m_uiRcvLen;             /*��ǰ���յĳ���*/
    unsigned int m_uiSndLen;             /*��ǰ���͵ĳ���*/

    //Bitmap_Header*  m_pReqBitmapHeader;   /*������Ϣͷ*/
    //Bitmap_Header*  m_pResBitmapHeader;   /*��Ӧ��Ϣͷ*/
    unsigned int m_uiRequestLen;             /*������Ϣ �ĳ���*/
    unsigned int m_uiResponseLen;          /*������Ϣ �ĳ���*/

    CCacheManage * m_pCCacheManage;  /*Cache ����ƹ�����ָ��*/

    unsigned char m_ucVersion;

};



#endif
