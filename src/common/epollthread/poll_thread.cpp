/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */



/*************************************************************************** 
 *       �ļ�����:  poll_thread.cpp
 *       �ļ�����:  ����������,�̳�epoll �����༰ʱ��������
                    1: ѭ���ȴ�epoll �¼�
                    2: ��鳬ʱ����
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.22 
 *       �޸ļ�¼:  
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
*��������: InitializeThread()
*�������: ��
*���ز���: ��
*��������: ��ʼ����ʱ���ʱ�估epoll  ������
************************************************************/
int CPollThread::InitializeThread()
{
    if(InitializePollerUnit() < 0)
    {
        return -1;
    }

    /*ÿ����賿4����*/
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
*��������: RunningThread()
*�������: ��
*���ز���: ��
*��������: ����epoll_wait ��ѭ��
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
            /*ÿ���賿��ʼִ��һ�������������ݵĳ�ʼ��*/
            if(((int)(g_pCServiceConf->m_uiScanBeginTime) == tm.tm_hour)&&(1 != iFlag))
            {
                /*������û����ݵ�ʱ��ϵͳ�쳣���������ٽ����û�
                   Ϊ�˱�������������������û����ݱ����ǵ�*/
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
                /*�Ѿ�������*/
                if(1 == iFlag)
                {
                    iFlag = 2;
                }
            }
            /*ÿ���賿��ʼִ��һ���䱸��ˮ��־�ı���*/
            if(((int)(g_pCServiceConf->m_uiColdBakBeginTime) == tm.tm_hour)&&(1 != iColdFlag))
            {
                CrotabColdDataBackup();
                iColdFlag = 1;
            }
            if(tm.tm_hour > (int)(g_pCServiceConf->m_uiColdBakBeginTime))
            {
                /*�Ѿ�������*/
                if(1 == iColdFlag)
                {
                    iColdFlag = 2;
                }
            }

        }
        /*ɨ�������ļ�����̬����*/
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
*��������: DeleteUnAliveLink()
*�������: ��
*���ز���: ��
*��������: �����·�ļ���ʱ�䣬�������24Сʱ����Ϣ����ɾ��
************************************************************/
inline void CPollThread::DeleteUnAliveLink(uint64_t now)
{
    /*δ������ʱ�䣬ֱ�ӷ���*/
    if(now < m_ulNextCheckTime)
    {
        return ;
    }

    /*�����´μ��ʱ��*/
    uint64_t ulTemp = 1000000;
    ulTemp =ulTemp *24*3600;
    m_ulNextCheckTime += ulTemp;

    /*��һ��epoll slot Ϊ�����˿�ռ�ݣ��������*/
    //  ǰ�漸��Ϊ�߳�fd��Ҳ������ʱ���
    unsigned int i;
    struct CEpollSlot * pCEpollSlot;
    for(i = 1; i < m_uiMaxPollers; i++)
    {
        pCEpollSlot = &m_pEpollerTable[i];
        if((NULL != pCEpollSlot->poller) &&(NULL == pCEpollSlot->freeList))
        {
            /*���ʱ��,��ʱ,����ó�ʱ����*/
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


