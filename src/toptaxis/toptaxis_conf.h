/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_conf.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP排序系统的配置文件
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_CONF_HEADER_H_
#define _TOPTAXIS_CONF_HEADER_H_
#include <vector>
#include <string>
#include <map>
#include <set>

#include "xmlParser.h"
#include "error_code.h"
#include "toptaxis_header.h"

using namespace std;

#define CONF_FILE_PATH                                           "../conf/toptaxis_conf.xml"
#define RULE_FILE_PATH                                           "../conf/rule_conf/"

#define MAX_CP_FILE_PATH_CMD                                     256
#define MAX_WHITE_LIST_NUM                                       20

/*Top排序服务器的类型*/
enum TAXIS_SERVER_TYPE
{
    E_TAXIS_TOP_SERVER     = 0,   //主TOP排序Server
    E_TAXIS_QUERY_SERVER   = 1,   //主TOP排序查询Server
    E_TAXIS_SERVER_TYPE_MAX,
};

typedef struct _stL5Info
{
    int iModId;
    int iCmd;
    unsigned int uiTimeOut;
}stL5Info;

typedef struct _Listen_Info
{
    char m_szProtocol[LISTEN_PROTOCOL_LEN];     //监听的协议
    char m_szListenIP[MAX_IP_ADDR_LEN];              //监听的ip
    unsigned int m_uiKeepAliveTime;                             //保持激活时间
    unsigned int m_uiTimeOut;                                    //超时时间
    unsigned int m_uiAcceptNum;                                 //同时处理的accept数目
}Listen_Info;

/*******************************************************************************
*  ClassName: CTaxisServiceConf
*  Date: 2011/8/31
*  Description: 获取top排序系统配置的类
*******************************************************************************/
class CTaxisServiceConf
{
public:
    CTaxisServiceConf();
    ~CTaxisServiceConf();
    int Init(const char * pProcessName);
    int Dump();
public:
    /*xml文件句柄*/
    XMLNode m_xmlConfigFile;

    char m_szWaterLogPath[MAX_FILE_PATH_LEN];

    Listen_Info m_struListenInfo;
    unsigned int m_uiMaxInputEpoll;

    unsigned int m_uiServerType;
    /*每天定时置换历史数据的时间*/
    unsigned int m_uiScanBeginTime; 
    /*每天定时冷备的时间*/
    unsigned int m_uiColdBakBeginTime; 
};
/*******************************************************************************
*  ClassName: CRuleConf
*  Date: 2011/8/31
*  Description: 获取top排序规则配置的类
*******************************************************************************/
class CRuleConf
{
public:
    CRuleConf();
    ~CRuleConf();
    int Init(const char * pConfName, unsigned int& uiConfBid);
    int ReloadConf(const char * pConfName, unsigned int& uiConfBid);
    int findsubstring(const char* p_src, char* p_subsrc, char* p_spliter);  
    int insert_subcond_vlist(vector <string> &v_condlist, string& str_data);
    int InsertWhiteList(V_WHITE_LIST &v_whitelist, string& str_data);
    void PrintLog();
public:
    XMLNode m_xmlConfigFile;

    map<unsigned short,stL5Info> m_mBid2L5;
    map<unsigned, map<string, unsigned short> > m_mApp2Attr;
    string m_strUinName;
    string m_strAppIDName;
    string m_strModName;
};

#endif


