/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */



/*************************************************************************** 
 *       文件名称:  poll_thread.cpp
 *       文件功能:  工作进程类,继承epoll 基础类及时间链表类
                    1: 循环等待epoll 事件
                    2: 检查超时连接
 *       文件作者:   
 *       修改时间:  2011.03.22 
 *       修改记录:  
 ***************************************************************************/

 
#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <string.h>


#include "poll_thread.h"
#include "log.h"
#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "toptaxis_conf.h"
#include "toptaxis_backup.h"
#include "toptaxis_sync.h"
#include "toptaxis_handle.h"
#include "send_warn.h"

extern CTaxisServiceConf*  g_pCServiceConf;
extern struct tm g_tmCoredump;

CPollThread::CPollThread(const char *szThreadName, unsigned int uiMaxPollers) :CPollerUnit(uiMaxPollers)
{
    strncpy(m_szThreadName, szThreadName, 32);
    m_uiTimeOut = 1000;
}

CPollThread::~CPollThread ()
{

}

/*************************************************************
*函数名称: InitializeThread()
*输入参数: 略
*返回参数: 略
*函数功能: 初始化超时检查时间及epoll  基础类
************************************************************/
int CPollThread::InitializeThread()
{
    if(InitializePollerUnit() < 0)
    {
        return -1;
    }

    /*每天的凌晨4点检查*/
    time_t  uiTime = time(NULL);
    struct tm* struNowTime = gmtime(&uiTime);
    struct tm struNextTime;
    memcpy(&struNextTime,struNowTime,sizeof(struct tm));

    struNextTime.tm_hour = 4;
    struNextTime.tm_min = 0;
    struNextTime.tm_sec = 0;

    m_ulNextCheckTime = mktime(&struNextTime)*1000000;

    return 0;
}

/*************************************************************
*函数名称: RunningThread()
*输入参数: 略
*返回参数: 略
*函数功能: 启动epoll_wait 并循环
************************************************************/
int CPollThread::RunningThread()
{
    time_t tNow;
    struct tm tm;
    int iFlag = 0, iColdFlag = 0;
    char str_warnmsg[MAX_WARN_LEN] = {0};
    while (true)
    {
        if(E_TAXIS_QUERY_SERVER != g_pCServiceConf->m_uiServerType)
        {
            tNow = time(NULL);
            localtime_r(&tNow, &tm);
            /*每天凌晨开始执行一次排序增量数据的初始化*/
            if(((int)(g_pCServiceConf->m_uiScanBeginTime) == tm.tm_hour)&&(1 != iFlag))
            {
                /*如果在置换数据的时候系统异常重启，则不再进行置换
                   为了避免多次重启，引发多次置换数据被覆盖掉*/
                if((g_tmCoredump.tm_year != tm.tm_year)
                ||(g_tmCoredump.tm_mon != tm.tm_mon)
                ||(g_tmCoredump.tm_mday != tm.tm_mday)
                ||(g_tmCoredump.tm_hour != tm.tm_hour)
                )
                {
                    CrotabUpdateHistoryData();
                }
                else
                {
                    WARN_LOG("CrotabUpdateHistoryData[fail]ChildProcessCoreDump");
                    snprintf(str_warnmsg, MAX_WARN_LEN, "Crotab Update History Data fail, toptaxis's process is coredump");
                    CWarnConf::Instance()->SendWarn(str_warnmsg);
                }
                iFlag = 1;
            }
            if(tm.tm_hour > (int)(g_pCServiceConf->m_uiScanBeginTime))
            {
                /*已经做过了*/
                if(1 == iFlag)
                {
                    iFlag = 2;
                }
            }
            /*每天凌晨开始执行一次冷备流水日志的备份*/
            if(((int)(g_pCServiceConf->m_uiColdBakBeginTime) == tm.tm_hour)&&(1 != iColdFlag))
            {
                CrotabColdDataBackup();
                iColdFlag = 1;
            }
            if(tm.tm_hour > (int)(g_pCServiceConf->m_uiColdBakBeginTime))
            {
                /*已经做过了*/
                if(1 == iColdFlag)
                {
                    iColdFlag = 2;
                }
            }

        }
        /*扫描配置文件并动态加载*/
        //ScanRuleSysConf();

        WaitPollerEvents(ExpireMicroSeconds(m_uiTimeOut));
        uint64_t now = GET_TIMESTAMP();

        ProcessPollerEvents();
        CheckExpired(now);
        //DeleteUnAliveLink(now);
        //CheckReady();
    }
    Cleanup();
    return 0;
}

/*************************************************************
*函数名称: DeleteUnAliveLink()
*输入参数: 略
*返回参数: 略
*函数功能: 检查链路的激活时间，如果超过24小时无消息，则删除
************************************************************/
inline void CPollThread::DeleteUnAliveLink(uint64_t now)
{
    /*未到达检查时间，直接返回*/
    if(now < m_ulNextCheckTime)
    {
        return ;
    }

    /*更新下次检查时间*/
    uint64_t ulTemp = 1000000;
    ulTemp =ulTemp *24*3600;
    m_ulNextCheckTime += ulTemp;

    /*第一个epoll slot 为监听端口占据，不做检查*/
    //  前面几个为线程fd，也不做超时检查
    unsigned int i;
    struct CEpollSlot * pCEpollSlot;
    for(i = 1; i < m_uiMaxPollers; i++)
    {
        pCEpollSlot = &m_pEpollerTable[i];
        if((NULL != pCEpollSlot->poller) &&(NULL == pCEpollSlot->freeList))
        {
            /*检查时间,超时,则调用超时函数*/
            if((now - pCEpollSlot->poller->m_ulLastAliveTime ) >  ulTemp)
            {
                ERROR_LOG("reason[close_link]timeout[]");
                pCEpollSlot->poller->HangupNotify();
            }
        }
    }
}
void CPollThread::Cleanup (void)
{
}


