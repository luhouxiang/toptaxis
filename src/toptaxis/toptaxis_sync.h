/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_sync.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP����ϵͳ��������ͬ���ĺ�������
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_SYNC_H_
#define _TOPTAXIS_SYNC_H_
#include <vector>
#include <string>
#include <map>
#include <set>
#include "tcp_client.h"

using namespace std;
/*******************************************************************************
*  ClassName: CNotifySync
*  Date: 2011/8/31
*  Description:  ֪ͨNotify��ȡ�����ڴ棬��������ͬ������
*******************************************************************************/
class CNotifySync
{
public:
        CNotifySync();
        virtual ~CNotifySync();

public:
        /*
             * desc: ʹ��apiǰ,��Ҫ������صĳ�ʼ��
             * @���������
             *iModId: L5�ı���ģ�����
             *iCmd:  L5�ı����ӿڱ���
             *iTimeOut: ��λ����
             */
    int Init(char* szIp, unsigned short usPort, int iTimeOut);
        /*
            * desc: ��ȡҵ������TOP����
            * @input:
            *   usBid: ҵ��id
            *uiRuleId: ����id
            *usCondId:����id
            *v_CondValue: ����id�ڸ��������Ķ�Ӧ��ֵ����Ҫ��ѯ�ģ�
            *                   ���磺����id��Ӧ�������ǡ�city/level��, ������ѯcityΪ1010��levelΪ3���û�
                                 �Ľ����top100, ����Ҫ�� 1010��3˳�����v_CondValue�� 
            * @output:
            *v_resultnode:  ��ȡ����TOP���򣨴�������1�������µ�Topֵ��
            *uiSerId: �ӹ����ļ�����͸������ѯ�̵�serid 
            *return value:  0���ɹ�
            *       others: ʧ�ܣ� ʧ�ܵĴ�����Ϣͨ��GetErrMsg������ȡ
        */
    int NotifySyncMsg(unsigned int uiShmKey);
    
private:
    CTaxisTCPClientSocket m_stTcpClient;
    char m_szIP[MAX_IP_ADDR_LEN];
    unsigned short m_usPort;

    char m_szSndBuf[MAX_REQUEST_MSG_LEN];// ͬ������buf
    char m_szRcvBuf[MAX_RESPONSE_MSG_LEN];  // ͬ����Ӧbuf

    int m_iTimeOut;// server��ʱʱ��

    int SendAndRecvByLocalConnect(char * send_buf, int send_len, char * recv_buf, int recv_maxLen, int &recv_len);

};
#endif


