/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_main.cpp
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description:   1: 加载配置文件
*                     2: 初始化日至系统
*                     3: 初始化epoll
*                     4: 全局变量的声明
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
#include "toptaxis_handle.h"
#include "comm_semp.h"
#include "ts_waterlog.h"


#ifndef __USE_GNU
#define __USE_GNU
#endif

#include<sched.h> 
#include<ctype.h> 
#include <map>

#include "dc_process.h"
#include "send_warn.h"

using namespace std;

#define MAX_WATERLOG_SIZE 2000000000

extern char g_szProcessName[255];

/*全局主进程类*/
CPollThread*  g_pWriteThread = NULL;

/*全局监听类*/
CListener*  g_pListenClass = NULL;

/*全局配置类*/
CTaxisServiceConf*  g_pCServiceConf = NULL;

/*规则配置类*/
CRuleConf*  g_pCRuleConf = NULL;

/*全局客户端管理类*/
CClientManage* g_pCClientManage = NULL;

/*记录流水数据的缓冲*/
char* g_sz_waterlogdata = NULL;

/*系统中业务数目*/
unsigned int g_uiBidNum = 0; 

/*系统中所有条件列表*/
vector <Bid_Condition> g_condition_list;
/*系统中规则数目*/
unsigned int g_uiRuleNum = 0; 
/*系统中所有的规则*/
MAP_TAXIS_RULE_LIST gMapRuleList;

/*系统中所有条件列表*/
vector <Bid_Condition> gReloadCondList;
/*系统中规则数目*/
unsigned int gReloadRuleNum = 0; 
/*系统中业务数目*/
unsigned int gReloadBidNum = 0; 



/*系统中bid与其在Bid_Condition中的index*/
map<string, unsigned int> g_map_bid_index;
/*系统中bid与其在Bid_Condition中的index*/
map<string, unsigned int> g_map_cond_index;

/*系统中所有条件列表*/
map<unsigned int, set<string> > g_cond_list;
/*系统中所有条件列表*/
map<unsigned int, set<string> > g_field_list; 
/*记录哪些用户在城市Top排行榜中  QQ号码， 城市编码。 
    城市变后则要先删除在旧城市的记录*/
map <unsigned int, unsigned int> g_user_top_list; 

/*记录用户在排行榜的位置*/
MAP_TAXIS_POS g_user_taxis_pos_list;

pthread_mutex_t g_mutex_taxishistory_map;

/*全量流水*/
CColdLog  g_toptaxis_cold_waterlog;

/*查询Server重启时记住重启的时间，方便流水完整同步*/
unsigned int g_ui_begin_time; 

/*全局cache 块管理类*/
CCacheManage*  g_pCCacheManage = NULL;

/*系统使用共享内存的起始Key*/
unsigned int g_ui_shm_max = 1000;

/*共享内存(多进程)操作的锁*/
CCommSemp g_cCommSemp;
/*通知Notify同步排序数据到备机*/
CNotifySync g_cNotifySync;
/*全局处理排序消息的对象*/
CTopTaxisHandle* g_pCTopTaxisHandle = NULL;

/*全局规则配置文件与Bid的关系映射表*/
MAP_TAXIS_RULECONF_BID g_MapRuleConfBid;

/*当前是否在处理消息标志, 便于每个消息只记录一次流水*/
unsigned int g_uiHandMsgFlag = 0;

/*统计Toptaxis当前性能负载的时间计数器*/
time_t g_tCountBeginTime = 0;

/*************************************************************
*函数名称: dump_conf()
*输入参数: 略
*返回参数: 略
*函数功能: 打印获取到的配置规则信息
************************************************************/
int dump_conf()
{
    unsigned int i =0;
    map<unsigned int, set<string> >::iterator itbegin;
    map<unsigned int, set<string> >::iterator itend;
    set<string>::iterator itsetbegin;
    set<string>::iterator itsetend;

     itbegin = g_cond_list.begin();
     itend = g_cond_list.end();
     INFO_LOG("CondList\n\n");
     for(; itbegin!=itend; itbegin++)
     {
        INFO_LOG("Bid[%u]", (unsigned int)itbegin->first);
        set<string> &set_temp = itbegin->second;
        itsetbegin = set_temp.begin();
        itsetend = set_temp.end();
        i = 0;
        for(; itsetbegin!=itsetend; itsetbegin++)
        {
            INFO_LOG("Cond%u[%s]", i++, (*itsetbegin).c_str());
        }
    }
    itbegin = g_field_list.begin();
    itend = g_field_list.end();
    for(; itbegin!=itend; itbegin++)
    {
        INFO_LOG("Bid[%u]", (unsigned int)itbegin->first);
        set<string> &set_temp = itbegin->second;
        itsetbegin = set_temp.begin();
        itsetend = set_temp.end();
        i = 0;
        for(; itsetbegin!=itsetend; itsetbegin++)
        {
            INFO_LOG("Field%u[%s]", i++, (*itsetbegin).c_str());
        }
    }
    return 0;
}
/*************************************************************
*函数名称: InitApp()
*输入参数: 略
*返回参数: 略
*函数功能: 工作进程入口函数
*          1: 初始化配置文件
*          2: 初始化与control的通讯类
************************************************************/
int InitApp()
{
    int iRet = 0;
    /*取当前时间(粒度到小时YYMMDDHH*/
    char curTimestr[MAX_NAME_LEN] = {0};
    struct tm curTmm;
    unsigned int uiRuleFileNum = 0, uiRuleFileIndex = 0;
    vector <string> vRuleFileList;
    time_t curTime = time(NULL);
    unsigned int uiConfBid = 0;
    g_tCountBeginTime = curTime;
    
    localtime_r((time_t*)&(curTime), &curTmm);
    snprintf(curTimestr, sizeof(curTimestr) - 1, "%04d%02d%02d%02d", 
    curTmm.tm_year + 1900, curTmm.tm_mon + 1, curTmm.tm_mday, curTmm.tm_hour);

    g_ui_begin_time = (unsigned int)strtoul(curTimestr, NULL, 10);

    /*加载系统配置文件*/
    g_pCServiceConf = new CTaxisServiceConf;
    if(NULL == g_pCServiceConf)
    {
        fprintf( stderr, "!!!!!!!!FAILED  [new TaxisServiceConf ]!!!!!!!!\n" );
        exit(E_EXIT_NORMAL);
    }

    if(g_pCServiceConf->Init(g_szProcessName))
    {
        fprintf( stderr, "!!!!!!!!FAILED  [Init TaxisServiceConf ]!!!!!!!!\n" );
        exit(E_EXIT_NORMAL);
    }
    /*加载规则配置文件*/
    g_pCRuleConf = new CRuleConf;
    if(NULL == g_pCRuleConf)
    {
        fprintf( stderr, "!!!!!!!!FAILED  [new TaxisRuleConf ]!!!!!!!!\n" );
        exit(E_EXIT_NORMAL);
    }

    vRuleFileList.clear();

    GetDirXmlFile(RULE_FILE_PATH, vRuleFileList);

    uiRuleFileNum = vRuleFileList.size();

    INFO_LOG("GetRuleFileNum[%u]", uiRuleFileNum);
    
    for(uiRuleFileIndex = 0; uiRuleFileIndex < uiRuleFileNum; uiRuleFileIndex++)
    {
        uiConfBid = 0;
        if(g_pCRuleConf->Init(vRuleFileList[uiRuleFileIndex].c_str(), uiConfBid))
        {
            fprintf( stderr, "!!!!!!!!FAILED  [Init TaxisRuleConf[%s] ]!!!!!!!!\n", 
                vRuleFileList[uiRuleFileIndex].c_str());
            exit(E_EXIT_NORMAL);
        }
        INFO_LOG("InitInsert[Conf-Bid]ConfFile[%s]Bid[%u]", vRuleFileList[uiRuleFileIndex].c_str(), uiConfBid);
        g_MapRuleConfBid.insert(make_pair(vRuleFileList[uiRuleFileIndex], uiConfBid));
    }
    
    /*打印读取到的配置规则*/
    dump_conf();

    g_pCRuleConf->PrintLog();

    if(CDataProcess::Instance()->Init())
    {
        EMERG_LOG("CDataProcess Init error");
        return -1;
    }
    
    /*流水日志初始化*/
    CTsWaterLog::Instance()->Init(g_pCServiceConf->m_szWaterLogPath, "water_", MAX_WATERLOG_SIZE);

    g_cCommSemp.Init(TAXIS_NOTIFY_SEMP_KEY);
    g_cNotifySync.Init(g_pCServiceConf->m_struListenInfo.m_szListenIP, TAXIS_NOTIFY_SOCKT_PORT, 1);

    g_sz_waterlogdata = new char[TAXIS_MAX_WATER_COLD_LOG];
    if(NULL == g_sz_waterlogdata)
    {
        BOOT_LOG_EN (1, "new  g_sz_waterlogdata  failed!!!!! ");
        exit(E_EXIT_NORMAL);
    }
    /*排序消息的对象的初始化*/
    g_pCTopTaxisHandle = new CTopTaxisHandle;
    if(NULL == g_pCTopTaxisHandle)
    {
        fprintf( stderr, "!!!!!!!!FAILED  [new CTopTaxisHandle ]!!!!!!!!\n" );
        exit(E_EXIT_NORMAL);
    }
    
    pthread_mutex_init(&g_mutex_taxishistory_map, NULL);
    /*先恢复数据*/
    RecoverData();
    
    iRet = EnterWork();

    BOOT_LOG_EN (1, "Init  EnterWork  failed!!!!! ");

    exit(E_EXIT_NORMAL);

}

/*************************************************************
*函数名称: EnterWork()
*输入参数: 略
*返回参数: 略
*函数功能: 入口函数
*          1: 完成所有工作类的初始化操作
*          2: 开启epoll 并wait
************************************************************/
int EnterWork()
{
    /*启动工作类*/
    StartThread();
    return ERROR_SYSTEM_FAILED;
}

/*************************************************************
*函数名称: StartThread()
*输入参数: 略
*返回参数: 略
*函数功能: 创建相关线程，监听及epoll
************************************************************/
int StartThread()
{
    Listen_Info* pListenInfo =  &g_pCServiceConf->m_struListenInfo;

    /*创建主工作线程类*/
    g_pWriteThread = new CPollThread(g_szProcessName, g_pCServiceConf->m_uiMaxInputEpoll);
    if(NULL == g_pWriteThread)
    {
        EMERG_LOG("new CPollThread memory failed");
        BOOT_LOG_EN (1, "NEW  CPollThread failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*初始化主工作线程*/
    if(g_pWriteThread->InitializeThread())
    {
        EMERG_LOG("g_pWriteThread.InitializeThread()failed");
        BOOT_LOG_EN (1, "INIT  CPollThread failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    NOTI_LOG("apply CPollThread  ---  main thread !");

    /*创建连接客户端管理类*/
    g_pCClientManage = new CClientManage(g_pWriteThread,pListenInfo->m_uiKeepAliveTime,pListenInfo->m_uiTimeOut);
    if(NULL == g_pCClientManage)
    {
        EMERG_LOG("g_pCClientManage new memory failed");
        BOOT_LOG_EN (1, "NEW  CClientManage failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    NOTI_LOG("apply CClientManage  ---  main work !");

    /*创建监听客户端子类*/
    g_pListenClass = new CListener(pListenInfo->m_szListenIP, E_PORT_TAXIS_WRITE, pListenInfo->m_uiAcceptNum, g_pCClientManage);
    if(NULL == g_pListenClass)
    {
        EMERG_LOG("g_pListenClass new memory failed");
        BOOT_LOG_EN (1, "NEW  CListener failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*启动监听*/
    if(g_pListenClass->Bind())
    {
        EMERG_LOG("g_pListenClass.Bind failed");
        BOOT_LOG_EN (1, "BIND  SOCKET failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*将监听类加入到epoll*/
    if(g_pListenClass->Attach(g_pWriteThread))
    {
        EMERG_LOG("g_pListenClass.Attach failed");
        BOOT_LOG_EN (1, "ATTACH  EPOLL failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }


    NOTI_LOG("start work !!!!!");
    BOOT_LOG_EN (0, "START WORKER!!!!! ");
    /*启动主线程，开始epoll  工作*/
    g_pWriteThread->RunningThread();

    return SUCCESS_CODE;
}
