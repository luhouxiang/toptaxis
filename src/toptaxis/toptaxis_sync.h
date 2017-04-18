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
*      Description: TOP排序系统排序数据同步的函数声明
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
*  Description:  通知Notify读取共享内存，进行数据同步的类
*******************************************************************************/
class CNotifySync
{
public:
        CNotifySync();
        virtual ~CNotifySync();

public:
        /*
             * desc: 使用api前,先要进行相关的初始化
             * @输入参数：
             *iModId: L5的被调模块编码
             *iCmd:  L5的被调接口编码
             *iTimeOut: 单位毫秒
             */
    int Init(char* szIp, unsigned short usPort, int iTimeOut);
        /*
            * desc: 获取业务规则的TOP排序
            * @input:
            *   usBid: 业务id
            *uiRuleId: 规则id
            *usCondId:条件id
            *v_CondValue: 条件id内各子条件的对应的值（需要查询的）
            *                   比如：条件id对应的条件是“city/level”, 如果想查询city为1010，level为3的用户
                                 的金币数top100, 则需要把 1010和3顺序放入v_CondValue中 
            * @output:
            *v_resultnode:  获取到的TOP排序（从排名第1依次往下到Top值）
            *uiSerId: 从规则文件里面透传给查询短的serid 
            *return value:  0：成功
            *       others: 失败， 失败的错误信息通过GetErrMsg函数获取
        */
    int NotifySyncMsg(unsigned int uiShmKey);
    
private:
    CTaxisTCPClientSocket m_stTcpClient;
    char m_szIP[MAX_IP_ADDR_LEN];
    unsigned short m_usPort;

    char m_szSndBuf[MAX_REQUEST_MSG_LEN];// 同步请求buf
    char m_szRcvBuf[MAX_RESPONSE_MSG_LEN];  // 同步响应buf

    int m_iTimeOut;// server超时时间

    int SendAndRecvByLocalConnect(char * send_buf, int send_len, char * recv_buf, int recv_maxLen, int &recv_len);

};
#endif


