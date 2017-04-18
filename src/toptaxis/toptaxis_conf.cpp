/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_conf.cpp
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description:   TOP排序系统的配置
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
#include "log.h"
#include "toptaxis_conf.h"
#include "toptaxis_main.h"
#include "toptaxis_header.h"

#define _get_xml_attr(x,y,z) (NULL == x.getAttribute(y) ? z : x.getAttribute(y)) 
/*系统中所有条件列表*/
extern vector <Bid_Condition> g_condition_list;
/*定时Reload的条件列表*/
extern vector <Bid_Condition> gReloadCondList;
/*系统中所有条件列表*/
extern map<unsigned int, set<string> > g_cond_list; 
/*系统中所有条件列表*/
extern map<unsigned int, set<string> > g_field_list; 
/*系统中业务数目*/
extern unsigned int g_uiBidNum; 

/*系统中所有的规则*/
extern MAP_TAXIS_RULE_LIST gMapRuleList;

CTaxisServiceConf::CTaxisServiceConf()
{
    memset(&m_struListenInfo, 0,sizeof(m_struListenInfo));

    memset(m_szWaterLogPath, 0, MAX_FILE_PATH_LEN);
    m_uiMaxInputEpoll = 1000;

    m_uiServerType = E_TAXIS_TOP_SERVER;
}
/*************************************************************
*函数名称: Dump()
*输入参数: 略
*返回参数: 略
*函数功能: 打印配置信息
************************************************************/
int CTaxisServiceConf::Dump()
{
    INFO_LOG("**************************************************");
    INFO_LOG(" water_path  [%s]", m_szWaterLogPath);

    INFO_LOG("");

    INFO_LOG(" toptaxis  info");
    INFO_LOG(" type   [%u]", m_uiServerType);
    INFO_LOG(" coldbak_begin_time   [%u]", m_uiColdBakBeginTime);
    INFO_LOG(" scan_begin_time   [%u]", m_uiScanBeginTime);

    INFO_LOG(" protocol   [%s]",m_struListenInfo.m_szProtocol);
    INFO_LOG(" listen_ip   [%s]",m_struListenInfo.m_szListenIP);
    INFO_LOG(" keep_a_time [%u]",m_struListenInfo.m_uiKeepAliveTime);
    INFO_LOG(" time_out   [%u]",m_struListenInfo.m_uiTimeOut);
    INFO_LOG(" accept_num  [%u]",m_struListenInfo.m_uiAcceptNum);

    INFO_LOG("");
    INFO_LOG(" max_epoll   [%u]",m_uiMaxInputEpoll);

    INFO_LOG("");

    return SUCCESS_CODE;
	
}
/*************************************************************
*函数名称: Init()
*输入参数: 略
*返回参数: 略
*函数功能: 加载配置文件
************************************************************/
int CTaxisServiceConf::Init(const char * pProcessName)
{
    /*检查输入的文件是否存在*/
    if (access (CONF_FILE_PATH, F_OK))
    {
        fprintf( stderr, "[Init] The config file is not exists\n" );
        return ERROR_OTHER_FAILED;
    }

    /*将配置文件加载到全局内存*/
    m_xmlConfigFile = XMLNode::openFileHelper(CONF_FILE_PATH, "content" ); 

    /*初始化日志系统*/
    XMLNode  xNode;

    xNode = m_xmlConfigFile.getChildNode( "log_info" ).getChildNode( "run_log" );
    if ((NULL == xNode.getAttribute("log_path"))
        ||(NULL == xNode.getAttribute("log_level"))
        ||(NULL == xNode.getAttribute("log_size")))
    {
        fprintf( stderr, "[InitialSystem] Invoid system_log parameter\n" );
        return ERROR_OTHER_FAILED;
    }

    log_init(xNode.getAttribute("log_path"), atoi( xNode.getAttribute("log_level")), 
        atoi(xNode.getAttribute("log_size")), pProcessName);


    /*初始化文件路径*/
    xNode = m_xmlConfigFile.getChildNode("log_info").getChildNode("data_log");
    strncpy(m_szWaterLogPath, xNode.getAttribute("water_path"), MAX_FILE_PATH_LEN);

    /*加载存储模块信息*/
    xNode = m_xmlConfigFile.getChildNode("toptaxis_moudle").getChildNode("listen_info");
    strncpy(m_struListenInfo.m_szProtocol, xNode.getAttribute("protocol"), LISTEN_PROTOCOL_LEN);
    strncpy(m_struListenInfo.m_szListenIP, xNode.getAttribute("listen_ip"), MAX_IP_ADDR_LEN);

    m_struListenInfo.m_uiKeepAliveTime = (unsigned int)atoi(xNode.getAttribute("keepalivetime"));
    m_struListenInfo.m_uiTimeOut = (unsigned int)atoi(xNode.getAttribute("timeout"));
    m_struListenInfo.m_uiAcceptNum = (unsigned int)atoi(xNode.getAttribute("accept_num"));

    xNode = m_xmlConfigFile.getChildNode( "toptaxis_moudle" ).getChildNode( "epoller_info" );
    m_uiMaxInputEpoll = (unsigned int)atoi(xNode.getAttribute("incomingpoller"));

    /*排序服务器的相关信息*/
    xNode = m_xmlConfigFile.getChildNode("toptaxis_moudle").getChildNode("server_info");
    m_uiServerType = 0;
    if(NULL != xNode.getAttribute("type"))
    {
        m_uiServerType =  atoi(xNode.getAttribute("type"));
    }
    m_uiScanBeginTime = 0;
    /*定时置换股票数据的时间*/
    if(NULL != xNode.getAttribute("scan_begin_time"))
    {
        m_uiScanBeginTime = atoi(xNode.getAttribute("scan_begin_time"));
    }

    m_uiColdBakBeginTime = 0;
    /*定时备份冷备全量流水的时间*/
    if(NULL != xNode.getAttribute("coldbak_begin_time"))
    {
        m_uiColdBakBeginTime = atoi(xNode.getAttribute("coldbak_begin_time"));
    }

    /*初始化文件路径*/
    Dump();

    return SUCCESS_CODE;
}

CRuleConf::CRuleConf()
{
    ;
}
/*************************************************************
*函数名称: InsertWhiteList()
*输入参数: 略
*返回参数: 略
*函数功能: 解析出规则条件里面的白名单
************************************************************/
int CRuleConf::InsertWhiteList(V_WHITE_LIST &v_whitelist, string& str_data)
{
    char* pToken = NULL;  
    unsigned long long ullValue = 0;
    unsigned int uiCount = 0;
    if(0 == str_data.length())
    {
        return 0;
    }
    int iLength = strlen(str_data.c_str());

    char* pTemp = new char[iLength+1];  
    bzero(pTemp, iLength+1);

    strncpy(pTemp, str_data.c_str(), iLength);  

    INFO_LOG("BeginWhiteList[%s]", pTemp);
    /* strtok会改变temp的值，所以上面作了拷贝字符串的工作*/
    pToken = strtok(pTemp, "|");            
    while(pToken)  
    {  
        INFO_LOG("PushCond[%s]", pToken);
        ullValue = strtoull(pToken, 0, 10);
        uiCount++;
        /*白名单超过最大数目，则不再加入*/
        if(uiCount > MAX_WHITE_LIST_NUM)
        {
            ERROR_LOG("WhiteList is OverMax[%u:%u], WHITENUMBER[%llu]Result[NoHandle]",
                MAX_WHITE_LIST_NUM, uiCount, ullValue);
            break;
        }
        v_whitelist.push_back(ullValue);  
        pToken = strtok(NULL, "|");  
    }  
    INFO_LOG("EndWhiteList");
    delete[] pTemp;  
    return 0;
}
/*************************************************************
*函数名称: insert_subcond_vlist()
*输入参数: 略
*返回参数: 略
*函数功能: 解析出规则条件里面各子条件
************************************************************/
int CRuleConf::insert_subcond_vlist(vector <string> &v_condlist, string& str_data)
{
    char* pToken = NULL; 
    if(0 == str_data.length())
    {
        return 0;
    }
    int iLength = strlen(str_data.c_str());

    char* pTemp = new char[iLength + 1];  
    bzero(pTemp, iLength + 1);

    strncpy(pTemp, str_data.c_str(), iLength);  

    INFO_LOG("BeginPushCond[%s]", pTemp);
    /* strtok会改变temp的值，所以上面作了拷贝字符串的工作*/
    pToken = strtok(pTemp, "|");            
    while(pToken)  
    {  
        /*week, day, system, user是属于系统字段*/
        if((0 != strcmp("week", pToken))
            &&(0 != strcmp("day", pToken))
            &&(0 != strcmp("system", pToken))
            &&(0 != strcmp("user", pToken)))
        {
            INFO_LOG("PushCond[%s]", pToken);
            v_condlist.push_back(pToken);  
        }
        pToken = strtok(NULL, "|");  
    }  
    INFO_LOG("EndPushCond");
    delete[] pTemp;  
    return 0;
}
/*************************************************************
*函数名称: findsubstring()
*输入参数: 略
*返回参数: 略
*函数功能: 把一个字符串按照分隔符分成子串
************************************************************/
int CRuleConf::findsubstring(const char* p_src, char* p_subsrc, char* p_spliter)  
{ 
    char* pToken = NULL;
    int iLen = 0;
    char* pTemp = NULL;  

    if((NULL == p_src)||(NULL == p_subsrc)||(NULL == p_spliter))
    {
        return -1;
    } 
    iLen = strlen(p_src);
    pTemp = new char[iLen+1];  

    strncpy(pTemp, p_src, iLen);  

    // strtok会改变temp的值，所以上面作了拷贝字符串的工作  
    pToken = strtok(pTemp, p_spliter);          

    while(pToken)  
    {  
        if(0 == strcmp(pToken, p_subsrc))
        {
            delete[] pTemp;
            return 0;
        }
        pToken = strtok(NULL, p_spliter);  
    }  
    delete[] pTemp;  
    return -1;
}  
/*************************************************************
*函数名称: Init()
*输入参数: pConfName: 规则配置文件的完整路径
*返回参数: 略
*函数功能: 加载规则文件
************************************************************/
int CRuleConf::Init(const char * pConfName, unsigned int& uiConfBid)
{
    XMLNode xmlConfigFile;
    unsigned int uiBid = 0;
    unsigned int uiCountTmp = 0, uiConditionsTmp = 0, i = 0, j = 0;
    string strTmp, strWhiteList, strWhiteListTmp; 
    Rule_Info stRuleTmp;
    Condition_Info stConditionTmp;
    Bid_Condition stBidConditionTmp;
    set <string> setCondList;
    set <string> setFieldList;
    char strPosFileName[MAX_FILE_PATH_LEN] = {0};
    char strTmpFileName[MAX_FILE_PATH_LEN] = {0};
    FILE* pFdTmp = NULL;
    int iRet = 0, iCount = 0;
    Toptaxis_Pos tRuleTopTaxisPos;
    unsigned int uiBidNum = 0, uiBidIndex = 0;

    bzero(&tRuleTopTaxisPos, sizeof(tRuleTopTaxisPos));

    if(NULL == pConfName)
    {
        ERROR_LOG("[Init] The rule config file[NULL] is not exists");
        return ERROR_OTHER_FAILED;
    }
    /*检查输入的文件是否存在*/
    if(access(pConfName, F_OK))
    {
        ERROR_LOG("[Init] The rule config file[%s] is not exists", pConfName);
        return ERROR_OTHER_FAILED;
    }

    INFO_LOG("BeginLoadRuleConf[%s]", pConfName);

    /*将配置文件加载到全局内存*/
    xmlConfigFile = XMLNode::openFileHelper(pConfName, "content"); 


    /*初始化日志系统*/
    XMLNode  xNode;
    if(NULL == xmlConfigFile.getAttribute("bid"))
    {
        ERROR_LOG("[rule.xml] Invoid bid parameter" );
        return ERROR_OTHER_FAILED;
    }
    /*读取规则文件的业务信息*/
    uiBid = (unsigned int)atoi(xmlConfigFile.getAttribute("bid"));

    uiConfBid = uiBid;
    
    uiBidNum = g_condition_list.size();
    for(uiBidIndex = 0; uiBidIndex < uiBidNum; uiBidIndex++)
    {
        if(uiBid == g_condition_list[uiBidIndex].m_uiBid)
        {
            ERROR_LOG("Bid[%u]Result[Repeat]", uiBid);
            return ERROR_OTHER_FAILED;    
        }
    }
    
    strWhiteListTmp.clear();
    stBidConditionTmp.m_vWhiteList.clear();
    stBidConditionTmp.m_uiBid = uiBid;
    if(NULL == xmlConfigFile.getAttribute("white_list"))
    {
        INFO_LOG("[rule.xml] bid white_list is null" );
    }
    else
    {
        strWhiteListTmp = string(xmlConfigFile.getAttribute("white_list"));
        InsertWhiteList(stBidConditionTmp.m_vWhiteList, strWhiteListTmp);
    }
    
    uiConditionsTmp = xmlConfigFile.nChildNode("bid_rule");
    /*读取规则文件的条件信息*/
    for(j = 0; j < uiConditionsTmp; j++)
    {
        bzero(&stConditionTmp, sizeof(stConditionTmp));
        xNode = xmlConfigFile.getChildNode("bid_rule", j);
        if ((NULL == xNode.getAttribute("conditions"))||
            (NULL == xNode.getAttribute("id"))||
            (NULL == xNode.getAttribute("serid")))
        {
            ERROR_LOG("[rule.xml] Invoid bid_rule[%u] parameter\n", j);
            return ERROR_OTHER_FAILED;
        }
        strTmp.clear();
        strTmp = string(xNode.getAttribute("conditions"));
        DEBUG_LOG("GetConditionFromConf[%s]", strTmp.c_str());
        stConditionTmp.m_usTaxisTime = E_REAL_DATA;
        if(0 == findsubstring(strTmp.c_str(), "day", "|"))
        {
            stConditionTmp.m_usTaxisTime = E_DAY_DATA;
        }
        if(0 == findsubstring(strTmp.c_str(), "week", "|"))
        {
            /*既有day 又有week，则返回失败*/
            if(1 == stConditionTmp.m_usTaxisTime)
            {
                ERROR_LOG("[rule.xml] Invoid day and week parameter\n");
                return ERROR_OTHER_FAILED;
            }
            stConditionTmp.m_usTaxisTime = E_WEEK_DATA;
        }

        strWhiteListTmp.clear();
        stConditionTmp.m_vWhiteList.clear();
        if(NULL == xNode.getAttribute("white_list"))
        {
            INFO_LOG("[rule.xml]condition white_list is null" );
        }
        else
        {
            strWhiteListTmp = string(xNode.getAttribute("white_list"));
            InsertWhiteList(stConditionTmp.m_vWhiteList, strWhiteListTmp);    
        }
        stConditionTmp.m_usCondId = (unsigned short)atoi(xNode.getAttribute("id"));
        stConditionTmp.m_uiSerId = (unsigned int)atoi(xNode.getAttribute("serid"));

        uiCountTmp =  xNode.nChildNode("rule");
        insert_subcond_vlist(stConditionTmp.m_vCondition, strTmp);

        unsigned int uiCondSize = stConditionTmp.m_vCondition.size();
        if(TAXIS_MAX_COND_NUM < uiCondSize)
        {
            ERROR_LOG("CondNum[%u]MaxCondNum[%u]", uiCondSize, TAXIS_MAX_COND_NUM);
            return ERROR_OTHER_FAILED;
        }
        for(i = 0; i < uiCondSize; i++)
        {
            setCondList.insert(stConditionTmp.m_vCondition[i]);
        }
        /*读取规则文件的规则信息*/
        for(i = 0; i < uiCountTmp; i++)
        {
            xNode = xmlConfigFile.getChildNode("bid_rule", j).getChildNode("rule", i);
            if ((NULL == xNode.getAttribute("ruleid"))||
                (NULL == xNode.getAttribute("field"))||
                (NULL == xNode.getAttribute("topnum"))||
                (NULL == xNode.getAttribute("overvalue"))||
                (NULL == xNode.getAttribute("type"))||
                (NULL == xNode.getAttribute("rule_enable"))
            )
            {
                ERROR_LOG("[rule.xml] Invoid rule parameter");
                return ERROR_OTHER_FAILED;
            }

            strWhiteListTmp.clear();
            stRuleTmp.m_vWhiteList.clear();

            if(NULL == xNode.getAttribute("white_list"))
            {
                INFO_LOG("[rule.xml]rule white_list is null" );
            }
            else
            {
                strWhiteListTmp = string(xNode.getAttribute("white_list"));
                InsertWhiteList(stRuleTmp.m_vWhiteList, strWhiteListTmp);   
            }
            
            stRuleTmp.m_uiRuleId = (unsigned int)atoi(xNode.getAttribute("ruleid"));
            stRuleTmp.m_sFieldId = (char*)(xNode.getAttribute("field"));
            setFieldList.insert(stRuleTmp.m_sFieldId);
            stRuleTmp.m_uiTopDisplayNum = (unsigned int)atoi(xNode.getAttribute("topnum"));
            stRuleTmp.m_uiTopNum = stRuleTmp.m_uiTopDisplayNum + (unsigned int)atoi(xNode.getAttribute("overvalue"));
            if(TAXIS_MAX_TOP_NUM < stRuleTmp.m_uiTopNum)
            {
                ERROR_LOG("TopNum[%u]MaxTopNum[%u]", stRuleTmp.m_uiTopNum, TAXIS_MAX_TOP_NUM);
                return ERROR_OTHER_FAILED;
            }
            stRuleTmp.m_usTaxisType = (unsigned short)atoi(xNode.getAttribute("type"));
            if(1 == atoi(xNode.getAttribute("rule_enable")))
            {
                /*此条规则生效*/
                stRuleTmp.m_cEnableFlag = E_RULE_ENABLE;
            }
            else
            {
                /*此条规则无效*/
                stRuleTmp.m_cEnableFlag = E_RULE_DISABLE;
            }
            
            iCount = gMapRuleList.count(stRuleTmp.m_uiRuleId);
            /*规则ID重复*/
            if(iCount > 0)
            {
                ERROR_LOG("RuleID[%u]Result[Repeat]", stRuleTmp.m_uiRuleId);
                continue;
            }
            tRuleTopTaxisPos.m_uiConditionListIndex = stBidConditionTmp.m_vConditionList.size();
            tRuleTopTaxisPos.m_uiRuleListIndex = stConditionTmp.m_vRuleList.size();
            /*系统所有的规则链表*/
            gMapRuleList.insert(make_pair(stRuleTmp.m_uiRuleId, tRuleTopTaxisPos));
            snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_LAST_DATA_DIR, TAXIS_LAST_DATA_PREFIX, 
                stRuleTmp.m_uiRuleId);
            if(access(strPosFileName, F_OK))
            {
                pFdTmp = fopen(strPosFileName, "w+"); 
            }
            else
            {
                pFdTmp = fopen(strPosFileName, "r+"); 
            }
            if(NULL == pFdTmp)
            {
                ERROR_LOG("openposfile[fail]filename[%s]error[%d:%s]", strPosFileName, errno, strerror(errno));
                return ERROR_OTHER_FAILED;
            }
            stRuleTmp.m_posfd = pFdTmp;
            /*按周或按天拍涨跌幅的需要记录历史数据*/
            while(E_REAL_DATA != stConditionTmp.m_usTaxisTime)
            {
                snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                    stRuleTmp.m_uiRuleId);
                /*文件不存在*/
                if(access(strPosFileName, F_OK))
                {
                    /*有可能在临时文件改名时重启了，从临时文件恢复*/
                    snprintf(strTmpFileName, MAX_FILE_PATH_LEN, "%s/%s%u%s", 
                        TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, stRuleTmp.m_uiRuleId, TAXIS_HISTORY_TMP_DATA_SUFFIX);
                    /*临时文件也不存在则创建新文件*/
                    if(access(strTmpFileName, F_OK))
                    {
                        pFdTmp = fopen(strPosFileName, "w+"); 
                    }
                    else
                    {
                        iRet = rename(strTmpFileName, strPosFileName);
                        if(0 != iRet)
                        {
                            ERROR_LOG("renamehistory[fail]fileorgname[%s]filedsc[%s]nameerror[%d:%s]", 
                                strTmpFileName, strPosFileName, errno, strerror(errno));
                            return ERROR_OTHER_FAILED;
                        }
                        /*重新读取*/
                        continue;
                    }
                }
                else
                {
                    pFdTmp = fopen(strPosFileName, "r+");
                }
                if(NULL == pFdTmp)
                {
                    ERROR_LOG("openhistoryfile[fail]filename[%s]error[%d:%s]", strPosFileName, errno, strerror(errno));
                    return ERROR_OTHER_FAILED;
                }
                /*正确的出口*/
                break;
            }
            stRuleTmp.m_historyfd = pFdTmp;
            stConditionTmp.m_vRuleList.push_back(stRuleTmp);
        }
        /*一个条件结束*/
        stBidConditionTmp.m_vConditionList.push_back(stConditionTmp);
    }
    g_cond_list.insert(make_pair(uiBid,setCondList));
    g_field_list.insert(make_pair(uiBid,setFieldList));
    g_condition_list.push_back(stBidConditionTmp);

    g_uiBidNum = g_condition_list.size();

    xNode = xmlConfigFile.getChildNode("server_list");
    int iSvrNum = xNode.nChildNode("server");
    stL5Info L5Temp;
    unsigned short usBid = 0;
    
    for(int i = 0;i < iSvrNum; i++)
    {
        XMLNode xServerNode=xNode.getChildNode("server",i);
        if(NULL == xServerNode.getAttribute("bid")
            ||NULL == xServerNode.getAttribute("modid")
            ||NULL == xServerNode.getAttribute("cmdid")
            ||NULL == xServerNode.getAttribute("timeout"))
        {
            ERROR_LOG("ReadCfg error tag server\n");
            return -1;
        }
        usBid = strtoul(xServerNode.getAttribute("bid"),NULL,10);

        L5Temp.iModId = atoi(xServerNode.getAttribute("modid"));
        L5Temp.iCmd = atoi(xServerNode.getAttribute("cmdid"));
        L5Temp.uiTimeOut = strtoul(xServerNode.getAttribute("timeout"),NULL,10);
        m_mBid2L5[usBid] = L5Temp;
    }

    xNode = xmlConfigFile.getChildNode("app_list");
    m_strUinName = _get_xml_attr(xNode,"field_key","uin");
    m_strAppIDName = _get_xml_attr(xNode,"field_appid","appid");
    m_strModName = _get_xml_attr(xNode,"field_modify","modifylist");
    int iAppNum = xNode.nChildNode("app");
    
    for(int i = 0;i < iAppNum; i++)
    {
        XMLNode xAppNode=xNode.getChildNode("app",i);
        
        if(NULL == xAppNode.getAttribute("appid"))
        {
            ERROR_LOG("ReadCfg error tag app\n");
            return -1;
        }
        
        unsigned uiAppID = strtoul(xAppNode.getAttribute("appid"),NULL,0);  
        map<string,unsigned short> mTemp;
        string strAttrName;
        int iAttrNum=xAppNode.nChildNode("attr");
        
        for(int i = 0;i < iAttrNum; i++)
        {
            XMLNode xAttrNode=xAppNode.getChildNode("attr",i);
            if(NULL == xAttrNode.getAttribute("attrname") || NULL == xAttrNode.getAttribute("bid"))
            {
                ERROR_LOG("ReadCfg error tag attr\n");
                return -1;
            }
            strAttrName = xAttrNode.getAttribute("attrname");
            usBid = strtoul(xAttrNode.getAttribute("bid"), NULL, 10);

            mTemp[strAttrName] = usBid;
        }
        m_mApp2Attr[uiAppID] = mTemp;
    }
    
    return SUCCESS_CODE;
}

/*************************************************************
*函数名称: ReloadConf()
*输入参数: pConfName: 规则配置文件全路径
*              uiBid: 该业务的Bid
*                     0 新规则配置文件，还没有Bid的
*                     其它值 规则配置文件的Bid
*返回参数: 略
*函数功能: 系统运行时实时扫描动态加载配置文件
************************************************************/
int CRuleConf::ReloadConf(const char * pConfName, unsigned int& uiConfBid)
{
    XMLNode xmlConfigFile;
    unsigned int uiBid = 0, uiBidNum = 0, uiBidIndex = 0;
    unsigned int uiCountTmp = 0, uiConditionsTmp = 0, i = 0, j = 0;
    string strTmp, strWhiteList, strWhiteListTmp; 
    Rule_Info stRuleTmp;
    Condition_Info stConditionTmp;
    Bid_Condition stBidConditionTmp;
    set <string> setCondList;
    set <string> setFieldList;
    char strPosFileName[MAX_FILE_PATH_LEN] = {0};
    char strTmpFileName[MAX_FILE_PATH_LEN] = {0};
    FILE* pFdTmp = NULL;
    int iRet = 0, iCount = 0, iNewConfFlag = 0;
    MAP_TAXIS_RULE_LIST::iterator itRuleList;

    if(NULL == pConfName)
    {
        ERROR_LOG("[Init] The rule config file[NULL] is not exists");
        return ERROR_OTHER_FAILED;
    }
    
    /*检查输入的文件是否存在*/
    if (access(pConfName, F_OK))
    {
        ERROR_LOG("[Init] The rule config file[%s] is not exists", pConfName);
        return ERROR_OTHER_FAILED;
    }

    INFO_LOG("BeginReloadRuleConf[%s]", pConfName);
    /*将配置文件加载到全局内存*/
    xmlConfigFile = XMLNode::openFileHelper(pConfName, "content"); 


    /*初始化日志系统*/
    XMLNode  xNode;
    if (NULL == xmlConfigFile.getAttribute("bid"))
    {
        ERROR_LOG("[rule.xml] Invoid bid parameter");
        return ERROR_OTHER_FAILED;
    }
    gReloadCondList.clear();

    /*读取规则文件的业务信息*/
    uiBid = (unsigned int)atoi(xmlConfigFile.getAttribute("bid"));
    /*已经成功导入的规则配置文件，要求Bid不能再被改动*/
    if(0 != uiConfBid)
    {
        if(uiBid != uiConfBid)
        {
            ERROR_LOG("Reload[Error]Bid[%u]ShouldBid[%u]", uiBid, uiConfBid);
            return ERROR_OTHER_FAILED;   
        }
    }
    /*新规则配置文件，还没有成功导入过的规则配置文件，需要确认配置文件的bid不能与已有的bid重复*/
    else
    {
        uiBidNum = g_condition_list.size();
        for(uiBidIndex = 0; uiBidIndex < uiBidNum; uiBidIndex++)
        {
            if(uiBid == g_condition_list[uiBidIndex].m_uiBid)
            {
                ERROR_LOG("ReloadBid[%u]Result[Repeat]", uiBid);
                return ERROR_OTHER_FAILED;    
            }
        }
        /*标识导入的是新配置文件*/
        iNewConfFlag = 1;
        uiConfBid = uiBid;
    }
    strWhiteListTmp.clear();
    stBidConditionTmp.m_vWhiteList.clear();
    stBidConditionTmp.m_uiBid = uiBid;
    if(NULL == xmlConfigFile.getAttribute("white_list"))
    {
        INFO_LOG("[rule.xml] bid white_list is null");
    }
    else
    {
        strWhiteListTmp = string(xmlConfigFile.getAttribute("white_list"));
        InsertWhiteList(stBidConditionTmp.m_vWhiteList, strWhiteListTmp);
    }
    
    uiConditionsTmp = xmlConfigFile.nChildNode("bid_rule");
    /*读取规则文件的条件信息*/
    for(j = 0; j < uiConditionsTmp; j++)
    {
        bzero(&stConditionTmp, sizeof(stConditionTmp));
        xNode = xmlConfigFile.getChildNode("bid_rule", j);
        if ((NULL == xNode.getAttribute("conditions"))||
            (NULL == xNode.getAttribute("id"))||
            (NULL == xNode.getAttribute("serid")))
        {
            ERROR_LOG("[rule.xml] Invoid bid_rule[%u] parameter\n", j);
            return ERROR_OTHER_FAILED;
        }
        strTmp.clear();
        strTmp = string(xNode.getAttribute("conditions"));
        DEBUG_LOG("GetConditionFromConf[%s]", strTmp.c_str());
 
        stConditionTmp.m_usTaxisTime = E_REAL_DATA;
        if(0 == findsubstring(strTmp.c_str(), "day", "|"))
        {
            stConditionTmp.m_usTaxisTime = E_DAY_DATA;
        }
        if(0 == findsubstring(strTmp.c_str(), "week", "|"))
        {
            /*既有day 又有week，则返回失败*/
            if(1 == stConditionTmp.m_usTaxisTime)
            {
                ERROR_LOG("[rule.xml] Invoid day and week parameter\n");
                return ERROR_OTHER_FAILED;
            }
            stConditionTmp.m_usTaxisTime = E_WEEK_DATA;
        }

        strWhiteListTmp.clear();
        stConditionTmp.m_vWhiteList.clear();
        if(NULL == xNode.getAttribute("white_list"))
        {
            INFO_LOG("[rule.xml]condition white_list is null\n" );
        }
        else
        {
            strWhiteListTmp = string(xNode.getAttribute("white_list"));
            InsertWhiteList(stConditionTmp.m_vWhiteList, strWhiteListTmp);    
        }
        
        stConditionTmp.m_usCondId = (unsigned short)atoi(xNode.getAttribute("id"));
        stConditionTmp.m_uiSerId = (unsigned int)atoi(xNode.getAttribute("serid"));
        uiCountTmp =  xNode.nChildNode("rule");
        insert_subcond_vlist(stConditionTmp.m_vCondition, strTmp);
 
        unsigned int uiCondSize = stConditionTmp.m_vCondition.size();
        if(TAXIS_MAX_COND_NUM < uiCondSize)
        {
            ERROR_LOG("CondNum[%u]MaxCondNum[%u]\n", uiCondSize, TAXIS_MAX_COND_NUM);
            return ERROR_OTHER_FAILED;
        }
        /*旧配置文件导入新规则配置*/
        if(0 == iNewConfFlag)
        {
            set <string>& rSetCondList = g_cond_list[uiBid];
            for(i = 0; i < uiCondSize; i++)
            {
                rSetCondList.insert(stConditionTmp.m_vCondition[i]);
            }
        }
        /*新配置文件导入*/
        else
        {
            for(i = 0; i < uiCondSize; i++)
            {
                setCondList.insert(stConditionTmp.m_vCondition[i]);
            }
        }
        /*读取规则文件的规则信息*/
        for(i = 0; i < uiCountTmp; i++)
        {
            xNode = xmlConfigFile.getChildNode("bid_rule", j).getChildNode("rule", i);
            if ((NULL == xNode.getAttribute("ruleid"))||
                (NULL == xNode.getAttribute("field"))||
                (NULL == xNode.getAttribute("topnum"))||
                (NULL == xNode.getAttribute("overvalue"))||
                (NULL == xNode.getAttribute("type"))||
                (NULL == xNode.getAttribute("rule_enable"))
            )
            {
                ERROR_LOG("[rule.xml] Invoid rule parameter");
                return ERROR_OTHER_FAILED;
            }

            strWhiteListTmp.clear();
            stRuleTmp.m_vWhiteList.clear();

            if(NULL == xNode.getAttribute("white_list"))
            {
                INFO_LOG("[rule.xml]rule white_list is null\n" );
            }
            else
            {
                strWhiteListTmp = string(xNode.getAttribute("white_list"));
                InsertWhiteList(stRuleTmp.m_vWhiteList, strWhiteListTmp);   
            }
            
            stRuleTmp.m_uiRuleId = (unsigned int)atoi(xNode.getAttribute("ruleid"));
            stRuleTmp.m_sFieldId = (char*)(xNode.getAttribute("field"));

            /*旧配置文件导入新规则配置*/
            if(0 == iNewConfFlag)
            {
                set <string>& rSetFieldList = g_field_list[uiBid];
                rSetFieldList.insert(stRuleTmp.m_sFieldId);
            }
            /*新配置文件导入*/
            else
            {
                setFieldList.insert(stRuleTmp.m_sFieldId);
            }
       
            stRuleTmp.m_uiTopDisplayNum = (unsigned int)atoi(xNode.getAttribute("topnum"));
            stRuleTmp.m_uiTopNum = stRuleTmp.m_uiTopDisplayNum + (unsigned int)atoi(xNode.getAttribute("overvalue"));
            if(TAXIS_MAX_TOP_NUM < stRuleTmp.m_uiTopNum)
            {
                ERROR_LOG("!!!TopNum[%u]MaxTopNum[%u]UseDisplay[%u]", stRuleTmp.m_uiTopNum, TAXIS_MAX_TOP_NUM, 
                    TAXIS_MAX_DISPLAY_TOP_NUM);
                stRuleTmp.m_uiTopNum = TAXIS_MAX_TOP_NUM;
                stRuleTmp.m_uiTopDisplayNum = TAXIS_MAX_DISPLAY_TOP_NUM;
                //return ERROR_OTHER_FAILED;
            }
            stRuleTmp.m_usTaxisType = (unsigned short)atoi(xNode.getAttribute("type"));
            if(1 == atoi(xNode.getAttribute("rule_enable")))
            {
                /*此条规则生效*/
                stRuleTmp.m_cEnableFlag = E_RULE_ENABLE;
            }
            else
            {
                /*此条规则无效*/
                stRuleTmp.m_cEnableFlag = E_RULE_DISABLE;
            }
            itRuleList = gMapRuleList.find(stRuleTmp.m_uiRuleId);
            if(itRuleList != gMapRuleList.end())
            {
                Toptaxis_Pos& rRuleTopTaxisPos = itRuleList->second;
                Bid_Condition &rBidCond = g_condition_list[rRuleTopTaxisPos.m_uiBidIndex];
                Condition_Info &rCondInfo = rBidCond.m_vConditionList[rRuleTopTaxisPos.m_uiConditionListIndex];
                Rule_Info& rRuleInfo = rCondInfo.m_vRuleList[rRuleTopTaxisPos.m_uiRuleListIndex];
                if(rRuleInfo.m_cEnableFlag != stRuleTmp.m_cEnableFlag)
                {
                    INFO_LOG("ReloadRule[%u]EnableFlag[%u]", rRuleInfo.m_uiRuleId, stRuleTmp.m_cEnableFlag);
                    rRuleInfo.m_cEnableFlag =  stRuleTmp.m_cEnableFlag;
                }
                if(rRuleInfo.m_uiTopDisplayNum != stRuleTmp.m_uiTopDisplayNum)
                {
                    INFO_LOG("ReloadRule[%u]TopDisplayNum[%u]", rRuleInfo.m_uiRuleId, stRuleTmp.m_uiTopDisplayNum);
                    rRuleInfo.m_uiTopDisplayNum =  stRuleTmp.m_uiTopDisplayNum;
                }
                if(rRuleInfo.m_uiTopNum != stRuleTmp.m_uiTopNum)
                {
                    INFO_LOG("ReloadRule[%u]TopNum[%u]", rRuleInfo.m_uiRuleId, stRuleTmp.m_uiTopNum);
                    rRuleInfo.m_uiTopNum = stRuleTmp.m_uiTopNum;
                }
                rRuleInfo.m_vWhiteList.clear();
                InsertWhiteList(rRuleInfo.m_vWhiteList, strWhiteListTmp);
                /*便于系统重新更新白名单，此规则不会重新导入到系统中*/
                stRuleTmp.m_uiRuleId = 0;
                stConditionTmp.m_vRuleList.push_back(stRuleTmp);                
                continue;
            }
            iCount = gMapRuleList.count(stRuleTmp.m_uiRuleId);
            /*规则ID不允许重复*/
            if(iCount > 0)
            {
                ERROR_LOG("RuleID[%u]Result[Repeat]", stRuleTmp.m_uiRuleId);
                continue;
            }

            snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_LAST_DATA_DIR, 
                TAXIS_LAST_DATA_PREFIX, stRuleTmp.m_uiRuleId);
            if(access(strPosFileName, F_OK))
            {
                pFdTmp = fopen(strPosFileName, "w+"); 
            }
            else
            {
                pFdTmp = fopen(strPosFileName, "r+"); 
            }
            if(NULL == pFdTmp)
            {
                ERROR_LOG("!!!!openposfile[fail]filename[%s]error[%d:%s]", strPosFileName, errno, strerror(errno));
                return ERROR_OTHER_FAILED;
            }
            stRuleTmp.m_posfd = pFdTmp;
            /*按周或按天拍涨跌幅的需要记录历史数据*/
            while(E_REAL_DATA != stConditionTmp.m_usTaxisTime)
            {
                snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                    stRuleTmp.m_uiRuleId);
                /*文件不存在*/
                if(access(strPosFileName, F_OK))
                {
                    /*有可能在临时文件改名时重启了，从临时文件恢复*/
                    snprintf(strTmpFileName, MAX_FILE_PATH_LEN, "%s/%s%u%s", 
                        TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, stRuleTmp.m_uiRuleId, TAXIS_HISTORY_TMP_DATA_SUFFIX);
                    /*临时文件也不存在则创建新文件*/
                    if(access(strTmpFileName, F_OK))
                    {
                        pFdTmp = fopen(strPosFileName, "w+"); 
                    }
                    else
                    {
                        iRet = rename(strTmpFileName, strPosFileName);
                        if(0 != iRet)
                        {
                            ERROR_LOG("!!!!!Renamehistory[fail]fileorgname[%s]filedsc[%s]nameerror[%d:%s]", 
                                strTmpFileName, strPosFileName, errno, strerror(errno));
                            usleep(1);
                        }
                        /*重新读取*/
                        continue;
                    }
                }
                else
                {
                    pFdTmp = fopen(strPosFileName, "r+");
                }
                if(NULL == pFdTmp)
                {
                    ERROR_LOG("!!!!Openhistoryfile[fail]filename[%s]error[%d:%s]", strPosFileName, errno, strerror(errno));
                    return ERROR_OTHER_FAILED;
                }
                /*正确的出口*/
                break;
            }
            stRuleTmp.m_historyfd = pFdTmp;
            stConditionTmp.m_vRuleList.push_back(stRuleTmp);
        }
        /*一个条件结束*/
        /*条件内没有规则，则不添加进去*/
        if(0 != stConditionTmp.m_vRuleList.size())
        {
            stBidConditionTmp.m_vConditionList.push_back(stConditionTmp);
        }
    }
    /*没有条件，则不添加进去*/
    if(0 != stBidConditionTmp.m_vConditionList.size())
    {
        gReloadCondList.push_back(stBidConditionTmp);
    }
    /*新配置文件需要新加入*/
    if(1 == iNewConfFlag)
    {
        g_cond_list.insert(make_pair(uiBid,setCondList));
        g_field_list.insert(make_pair(uiBid,setFieldList));
    }

    xNode = xmlConfigFile.getChildNode("server_list");
    int iSvrNum = xNode.nChildNode("server");
    stL5Info L5Temp;
    unsigned short usBid = 0;
    
    for(int i = 0;i < iSvrNum; i++)
    {
        XMLNode xServerNode=xNode.getChildNode("server",i);
        if(NULL == xServerNode.getAttribute("bid")
            ||NULL == xServerNode.getAttribute("modid")
            ||NULL == xServerNode.getAttribute("cmdid")
            ||NULL == xServerNode.getAttribute("timeout"))
        {
            ERROR_LOG("ReadCfg error tag server\n");
            return -1;
        }
        usBid = strtoul(xServerNode.getAttribute("bid"),NULL,10);

        L5Temp.iModId = atoi(xServerNode.getAttribute("modid"));
        L5Temp.iCmd = atoi(xServerNode.getAttribute("cmdid"));
        L5Temp.uiTimeOut = strtoul(xServerNode.getAttribute("timeout"),NULL,10);
        m_mBid2L5[usBid] = L5Temp;
    }

    xNode = xmlConfigFile.getChildNode("app_list");
    m_strUinName = _get_xml_attr(xNode,"field_key","uin");
    m_strAppIDName = _get_xml_attr(xNode,"field_appid","appid");
    m_strModName = _get_xml_attr(xNode,"field_modify","modifylist");
    int iAppNum = xNode.nChildNode("app");
    
    for(int i = 0;i < iAppNum; i++)
    {
        XMLNode xAppNode=xNode.getChildNode("app",i);
        
        if(NULL == xAppNode.getAttribute("appid"))
        {
            ERROR_LOG("ReadCfg error tag app\n");
            return -1;
        }
        
        unsigned uiAppID = strtoul(xAppNode.getAttribute("appid"),NULL,0);  
        map<string,unsigned short> mTemp;
        string strAttrName;
        int iAttrNum=xAppNode.nChildNode("attr");
        
        for(int i = 0;i < iAttrNum; i++)
        {
            XMLNode xAttrNode=xAppNode.getChildNode("attr",i);
            if(NULL == xAttrNode.getAttribute("attrname") || NULL == xAttrNode.getAttribute("bid"))
            {
                ERROR_LOG("ReadCfg error tag attr\n");
                return -1;
            }
            strAttrName = xAttrNode.getAttribute("attrname");
            usBid = strtoul(xAttrNode.getAttribute("bid"), NULL, 10);

            mTemp[strAttrName] = usBid;
        }
        m_mApp2Attr[uiAppID] = mTemp;
    }
    
    return SUCCESS_CODE;
}

/*************************************************************
*函数名称: PrintLog()
*输入参数:
*返回参数: 
*函数功能: 调试信息，打印读取的配置数据
************************************************************/
void CRuleConf::PrintLog()
{
    WARN_LOG( "conf_begin");

    stL5Info L5Temp;
    map<unsigned short,stL5Info>::iterator it;
    
    for(it = m_mBid2L5.begin(); it != m_mBid2L5.end(); it++)
    {
        L5Temp = it->second;
        WARN_LOG( "bid[%u]modid[%d]cmdid[%d]timeout[%u]",
            it->first, L5Temp.iModId, L5Temp.iCmd, L5Temp.uiTimeOut);
    }

    map<unsigned,map<string,unsigned short> >::iterator itApp2Attr;
    map<string,unsigned short>::iterator itAttr2Bid;
    map<string,unsigned short> mTemp;
    
    for(itApp2Attr = m_mApp2Attr.begin(); itApp2Attr != m_mApp2Attr.end(); itApp2Attr++)
    {
        mTemp = itApp2Attr->second;
        for(itAttr2Bid = mTemp.begin(); itAttr2Bid != mTemp.end(); itAttr2Bid++)
        {
            WARN_LOG( "appid[%u]attrname[%s]bid[%u]",
            itApp2Attr->first,(itAttr2Bid->first).c_str(), itAttr2Bid->second);
        }
    }

    WARN_LOG( "conf_end");
}

