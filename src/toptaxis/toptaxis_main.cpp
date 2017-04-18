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
*      Description:   1: ���������ļ�
*                     2: ��ʼ������ϵͳ
*                     3: ��ʼ��epoll
*                     4: ȫ�ֱ���������
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

/*ȫ����������*/
CPollThread*  g_pWriteThread = NULL;

/*ȫ�ּ�����*/
CListener*  g_pListenClass = NULL;

/*ȫ��������*/
CTaxisServiceConf*  g_pCServiceConf = NULL;

/*����������*/
CRuleConf*  g_pCRuleConf = NULL;

/*ȫ�ֿͻ��˹�����*/
CClientManage* g_pCClientManage = NULL;

/*��¼��ˮ���ݵĻ���*/
char* g_sz_waterlogdata = NULL;

/*ϵͳ��ҵ����Ŀ*/
unsigned int g_uiBidNum = 0; 

/*ϵͳ�����������б�*/
vector <Bid_Condition> g_condition_list;
/*ϵͳ�й�����Ŀ*/
unsigned int g_uiRuleNum = 0; 
/*ϵͳ�����еĹ���*/
MAP_TAXIS_RULE_LIST gMapRuleList;

/*ϵͳ�����������б�*/
vector <Bid_Condition> gReloadCondList;
/*ϵͳ�й�����Ŀ*/
unsigned int gReloadRuleNum = 0; 
/*ϵͳ��ҵ����Ŀ*/
unsigned int gReloadBidNum = 0; 



/*ϵͳ��bid������Bid_Condition�е�index*/
map<string, unsigned int> g_map_bid_index;
/*ϵͳ��bid������Bid_Condition�е�index*/
map<string, unsigned int> g_map_cond_index;

/*ϵͳ�����������б�*/
map<unsigned int, set<string> > g_cond_list;
/*ϵͳ�����������б�*/
map<unsigned int, set<string> > g_field_list; 
/*��¼��Щ�û��ڳ���Top���а���  QQ���룬 ���б��롣 
    ���б����Ҫ��ɾ���ھɳ��еļ�¼*/
map <unsigned int, unsigned int> g_user_top_list; 

/*��¼�û������а��λ��*/
MAP_TAXIS_POS g_user_taxis_pos_list;

pthread_mutex_t g_mutex_taxishistory_map;

/*ȫ����ˮ*/
CColdLog  g_toptaxis_cold_waterlog;

/*��ѯServer����ʱ��ס������ʱ�䣬������ˮ����ͬ��*/
unsigned int g_ui_begin_time; 

/*ȫ��cache �������*/
CCacheManage*  g_pCCacheManage = NULL;

/*ϵͳʹ�ù����ڴ����ʼKey*/
unsigned int g_ui_shm_max = 1000;

/*�����ڴ�(�����)��������*/
CCommSemp g_cCommSemp;
/*֪ͨNotifyͬ���������ݵ�����*/
CNotifySync g_cNotifySync;
/*ȫ�ִ���������Ϣ�Ķ���*/
CTopTaxisHandle* g_pCTopTaxisHandle = NULL;

/*ȫ�ֹ��������ļ���Bid�Ĺ�ϵӳ���*/
MAP_TAXIS_RULECONF_BID g_MapRuleConfBid;

/*��ǰ�Ƿ��ڴ�����Ϣ��־, ����ÿ����Ϣֻ��¼һ����ˮ*/
unsigned int g_uiHandMsgFlag = 0;

/*ͳ��Toptaxis��ǰ���ܸ��ص�ʱ�������*/
time_t g_tCountBeginTime = 0;

/*************************************************************
*��������: dump_conf()
*�������: ��
*���ز���: ��
*��������: ��ӡ��ȡ�������ù�����Ϣ
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
*��������: InitApp()
*�������: ��
*���ز���: ��
*��������: ����������ں���
*          1: ��ʼ�������ļ�
*          2: ��ʼ����control��ͨѶ��
************************************************************/
int InitApp()
{
    int iRet = 0;
    /*ȡ��ǰʱ��(���ȵ�СʱYYMMDDHH*/
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

    /*����ϵͳ�����ļ�*/
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
    /*���ع��������ļ�*/
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
    
    /*��ӡ��ȡ�������ù���*/
    dump_conf();

    g_pCRuleConf->PrintLog();

    if(CDataProcess::Instance()->Init())
    {
        EMERG_LOG("CDataProcess Init error");
        return -1;
    }
    
    /*��ˮ��־��ʼ��*/
    CTsWaterLog::Instance()->Init(g_pCServiceConf->m_szWaterLogPath, "water_", MAX_WATERLOG_SIZE);

    g_cCommSemp.Init(TAXIS_NOTIFY_SEMP_KEY);
    g_cNotifySync.Init(g_pCServiceConf->m_struListenInfo.m_szListenIP, TAXIS_NOTIFY_SOCKT_PORT, 1);

    g_sz_waterlogdata = new char[TAXIS_MAX_WATER_COLD_LOG];
    if(NULL == g_sz_waterlogdata)
    {
        BOOT_LOG_EN (1, "new  g_sz_waterlogdata  failed!!!!! ");
        exit(E_EXIT_NORMAL);
    }
    /*������Ϣ�Ķ���ĳ�ʼ��*/
    g_pCTopTaxisHandle = new CTopTaxisHandle;
    if(NULL == g_pCTopTaxisHandle)
    {
        fprintf( stderr, "!!!!!!!!FAILED  [new CTopTaxisHandle ]!!!!!!!!\n" );
        exit(E_EXIT_NORMAL);
    }
    
    pthread_mutex_init(&g_mutex_taxishistory_map, NULL);
    /*�Ȼָ�����*/
    RecoverData();
    
    iRet = EnterWork();

    BOOT_LOG_EN (1, "Init  EnterWork  failed!!!!! ");

    exit(E_EXIT_NORMAL);

}

/*************************************************************
*��������: EnterWork()
*�������: ��
*���ز���: ��
*��������: ��ں���
*          1: ������й�����ĳ�ʼ������
*          2: ����epoll ��wait
************************************************************/
int EnterWork()
{
    /*����������*/
    StartThread();
    return ERROR_SYSTEM_FAILED;
}

/*************************************************************
*��������: StartThread()
*�������: ��
*���ز���: ��
*��������: ��������̣߳�������epoll
************************************************************/
int StartThread()
{
    Listen_Info* pListenInfo =  &g_pCServiceConf->m_struListenInfo;

    /*�����������߳���*/
    g_pWriteThread = new CPollThread(g_szProcessName, g_pCServiceConf->m_uiMaxInputEpoll);
    if(NULL == g_pWriteThread)
    {
        EMERG_LOG("new CPollThread memory failed");
        BOOT_LOG_EN (1, "NEW  CPollThread failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*��ʼ���������߳�*/
    if(g_pWriteThread->InitializeThread())
    {
        EMERG_LOG("g_pWriteThread.InitializeThread()failed");
        BOOT_LOG_EN (1, "INIT  CPollThread failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    NOTI_LOG("apply CPollThread  ---  main thread !");

    /*�������ӿͻ��˹�����*/
    g_pCClientManage = new CClientManage(g_pWriteThread,pListenInfo->m_uiKeepAliveTime,pListenInfo->m_uiTimeOut);
    if(NULL == g_pCClientManage)
    {
        EMERG_LOG("g_pCClientManage new memory failed");
        BOOT_LOG_EN (1, "NEW  CClientManage failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    NOTI_LOG("apply CClientManage  ---  main work !");

    /*���������ͻ�������*/
    g_pListenClass = new CListener(pListenInfo->m_szListenIP, E_PORT_TAXIS_WRITE, pListenInfo->m_uiAcceptNum, g_pCClientManage);
    if(NULL == g_pListenClass)
    {
        EMERG_LOG("g_pListenClass new memory failed");
        BOOT_LOG_EN (1, "NEW  CListener failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*��������*/
    if(g_pListenClass->Bind())
    {
        EMERG_LOG("g_pListenClass.Bind failed");
        BOOT_LOG_EN (1, "BIND  SOCKET failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }

    /*����������뵽epoll*/
    if(g_pListenClass->Attach(g_pWriteThread))
    {
        EMERG_LOG("g_pListenClass.Attach failed");
        BOOT_LOG_EN (1, "ATTACH  EPOLL failed!!!!! ");
        return ERROR_SYSTEM_FAILED;
    }


    NOTI_LOG("start work !!!!!");
    BOOT_LOG_EN (0, "START WORKER!!!!! ");
    /*�������̣߳���ʼepoll  ����*/
    g_pWriteThread->RunningThread();

    return SUCCESS_CODE;
}
