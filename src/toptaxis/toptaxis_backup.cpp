/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_backup.cpp
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description:   TOP����ϵͳϵͳ���ݱ��ݻָ�
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

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include<sched.h> 
#include<ctype.h> 
#include <map>

#include "dc_process.h"
#include "send_warn.h"
using namespace std;

/*ϵͳ�����������б�*/
extern vector <Bid_Condition> g_condition_list;
/*ϵͳ�����������б�*/
extern map<unsigned int, set<string> > g_cond_list; 
/*ϵͳ�����й����б�*/
extern map<unsigned int, set<string> > g_field_list; 
/*ϵͳ��ҵ����Ŀ*/
extern unsigned int g_uiBidNum; 
extern pthread_mutex_t g_mutex_taxishistory_map;

/*��¼�û������а��λ��*/
extern MAP_TAXIS_POS g_user_taxis_pos_list;

/*�����ڴ�(�����)��������*/
extern CCommSemp g_cCommSemp;

/*��¼��ˮ���ݵĻ���*/
extern char* g_sz_waterlogdata;

/*֪ͨNotifyͬ���������ݵ�����*/
extern CNotifySync g_cNotifySync;

/*ϵͳʹ�ù����ڴ����ʼKey*/
extern unsigned int g_ui_shm_max;

/*ȫ����ˮ*/
extern CColdLog  g_toptaxis_cold_waterlog;

/*ȫ��������*/
extern CTaxisServiceConf*  g_pCServiceConf;

/*ȫ�ִ���������Ϣ�Ķ���*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

/*��ǰ�Ƿ��ڴ�����Ϣ��־, ����ÿ����Ϣֻ��¼һ����ˮ*/
extern unsigned int g_uiHandMsgFlag;

CColdLog::CColdLog() 
{
    memset(m_szLogPath, 0, sizeof(m_szLogPath));
    m_iLogfd = ERR_FD_NO;
}
/*************************************************************
*��������: Init()
*�������: szlogpath:д�䱸��ˮ�ļ�������·��
*���ز���:     0 : �ɹ�
*          ����ֵ: ʧ��
*��������: д��ǰ�������������ݵ������ļ�
************************************************************/
int CColdLog::Init(char *szlogpath)
{
    m_iLogfd = open(szlogpath, O_CREAT | O_RDWR | O_APPEND, 0666);

    if(m_iLogfd > 0)
    {
        return 0;
    }
    else
    {
        ERROR_LOG("open file[%s] in append mode fail!", szlogpath);
        m_iLogfd = ERR_FD_NO;
        return -1;
    }
}
/*************************************************************
*��������: WriteLog()
*�������: data    :�䱸��ˮ������
*          datalen :�䱸��ˮ�����ݳ���
*���ز���: ���ڵ���0 : д�ļ������ݸ���
*          ����ֵ: ʧ��
*��������: д��ǰ�������������ݵ������ļ�
************************************************************/
int CColdLog::WriteLog(const char * data, unsigned int datalen)
{
    int retLen = 0;

    /* check fd*/
    /* �ϴ�дʧ�ܵ�ʱ�� fd ���ÿ�*/
    if(ERR_FD_NO == m_iLogfd) 
    {
        ERROR_LOG("coldfd[NULL]data[%s]", data);
        return -1;
    }

    /* write data*/
    if((retLen = write(m_iLogfd, data, datalen)) < (int)datalen)
    {
        ERROR_LOG("cold write file err. retLen[%d] should[%u]data[%s]", retLen, datalen, data);
        return -2;
    }
    /* ����һ��д������ݳ���*/
    return retLen; 
}
/*************************************************************
*��������: CloseFile()
*�������: ��
*���ز���: ��
*��������: �ر��䱸�ļ�
************************************************************/
void CColdLog::CloseFile()
{
    if (m_iLogfd> 0)
    {
        close(m_iLogfd);
        m_iLogfd= ERR_FD_NO;
    }
}
/*************************************************************
*��������: ~CColdLog()
*�������: ��
*���ز���: ��
*��������: �ͷ���Դ
************************************************************/
CColdLog::~CColdLog() 
{
    memset(m_szLogPath, 0, sizeof(m_szLogPath));

    if(m_iLogfd > 0)
    {
        close(m_iLogfd);
        m_iLogfd = ERR_FD_NO;
    }
}
/*************************************************************
*��������: CrotabUpdateHistoryData()
*�������: ��
*���ز���: ��
*��������: ���µ���ĳ�ʼֵ
************************************************************/
int CrotabUpdateHistoryData(void)
{
    MAP_HISTORY_VALUE::iterator pMapHistory;
    All_History_Value* pHistory = NULL;
    unsigned int i = 0, j = 0, k = 0, uiConTmp = 0, uiRuleTmp = 0;
    unsigned int uiHistoryValue[WEEK_DAYS] = {0};
    unsigned int iSize = 0, n = 0;
    char strTmpFileName[MAX_FILE_PATH_LEN] = {0};
    char strPosFileName[MAX_FILE_PATH_LEN] = {0};
    char szTmpData[TAXIS_HISTORY_DATA_MAX_LEN] = {0};
    char szUidata[TAXIS_MAX_UNSIGN_INT] = {0};
    FILE* pFdTmp = NULL, *pFdOldTmp = NULL;
    Toptaxis_Msg stToptaxisMsgTmp;
    Field_Info stFieldInfoTmpRule;

    //pthread_mutex_lock( &g_mutex_taxishistory_map);
    for(i = 0; i < g_uiBidNum; i++)
    {
        uiConTmp = g_condition_list[i].m_vConditionList.size();
        for(j = 0; j < uiConTmp; j++)
        {
            if(E_DAY_DATA == g_condition_list[i].m_vConditionList[j].m_usTaxisTime)
            {
                uiRuleTmp = g_condition_list[i].m_vConditionList[j].m_vRuleList.size();
                for(k = 0; k < uiRuleTmp; k++)
                {
                    /*�ȰѴ˹�������������������*/
                    /*����Ҫ��գ����������week��hand_msg��Ϣ������մ���*/
                    snprintf(strTmpFileName, MAX_FILE_PATH_LEN, "%s/%s%u%s", 
                        TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                        g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_uiRuleId, TAXIS_HISTORY_TMP_DATA_SUFFIX);
                        snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                        g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_uiRuleId);
                    pFdTmp = fopen(strTmpFileName, "w+"); 
                    if(NULL == pFdTmp)
                    {
                        ERROR_LOG("openfail[%s]", strTmpFileName);
                    }
                    for(pMapHistory = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.begin(); 
                        pMapHistory != g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.end(); 
                        pMapHistory++)
                    {
                        pHistory = (All_History_Value*)&(pMapHistory->second);
                        /*���³�ʼֵΪ����仯������ֵ*/
                        pHistory->m_vuiInitData[0] = pHistory->m_uiCurData;
                        snprintf(szTmpData, TAXIS_HISTORY_DATA_MAX_LEN, "%llu_%u_%u\n",
                            pMapHistory->first, 
                            pHistory->m_vuiInitData[0], 
                            pHistory->m_vuiInitData[0]);
                        if(NULL != pFdTmp)
                        {
                            fwrite(szTmpData, 1, strlen(szTmpData), pFdTmp);
                        }
                    }
                    if(0 != fclose(pFdTmp))
                    {
                        ERROR_LOG("closefile[fail]filename[%s]error[%d:%s]\n", 
                        strTmpFileName, errno, strerror(errno));
                        //continue;
                    }
                    pFdOldTmp = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_historyfd;
                    if(NULL != pFdOldTmp)
                    {
                        if(0 != fclose(pFdOldTmp))
                        {
                            ERROR_LOG("closefile[fail]filename[%s]error[%d:%s]\n", 
                            strPosFileName, errno, strerror(errno));
                            //continue;
                        }
                    }
                    if(0 != unlink(strPosFileName))
                    {
                        ERROR_LOG("delfile[fail]filename[%s]error[%d:%s]\n", 
                        strPosFileName, errno, strerror(errno));	
                        //continue;
                    }
                    if(0 !=rename(strTmpFileName, strPosFileName))
                    {
                        ERROR_LOG("renamehistory[fail]fileorgname[%s]filedsc[%s]nameerror[%d:%s]\n", 
                        strTmpFileName, strPosFileName, errno, strerror(errno));	
                        //continue;
                    }
                    /*���ļ�������Ϣ�����̼߳���д��������*/
                    pFdOldTmp = fopen(strPosFileName, "r+"); 
                    if(NULL == pFdOldTmp)
                    {
                        ERROR_LOG("openfail[%s]", strPosFileName);
                    }
                    if(0 !=fseek(pFdOldTmp, 0L, SEEK_END))
                    {
                        ERROR_LOG("fseek[fail]filename[%s]seekpos[SEEK_END]error[%d:%s]\n", 
                        strPosFileName, errno, strerror(errno));
                        /*ȷ�����ݲ��ܱ�����*/
                        continue;
                    }
                    g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_historyfd = pFdOldTmp;
                }
            }
            else if(E_WEEK_DATA == g_condition_list[i].m_vConditionList[j].m_usTaxisTime)
            {
                /*����������ע��*/
                stToptaxisMsgTmp.m_uiBid = g_condition_list[i].m_uiBid;
                stToptaxisMsgTmp.m_usCondChange = 0;

                uiRuleTmp = g_condition_list[i].m_vConditionList[j].m_vRuleList.size();
                for(k = 0; k < uiRuleTmp; k++)
                {
                    /*�ȰѴ˹�������������������*/
                    snprintf(strTmpFileName, MAX_FILE_PATH_LEN, "%s/%s%u%s", 
                    TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                    g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_uiRuleId, TAXIS_HISTORY_TMP_DATA_SUFFIX);
                    snprintf(strPosFileName, MAX_FILE_PATH_LEN, "%s/%s%u", TAXIS_HISTORY_DATA_DIR, TAXIS_HISTORY_DATA_PREFIX, 
                        g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_uiRuleId);
                    pFdTmp = fopen(strTmpFileName, "w+"); 
                    if(NULL == pFdTmp)
                    {
                        ERROR_LOG("openfail[%s]", strTmpFileName);
                    }
                    /*����������ע��*/
                    stFieldInfoTmpRule.m_sField = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_sFieldId;

                    for(pMapHistory = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.begin(); 
                        pMapHistory != g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.end(); 
                        pMapHistory++)
                    {
                        pHistory = (All_History_Value*)&(pMapHistory->second);
                        /*���³�ʼֵΪ����仯������ֵ*/
                        pHistory->m_vuiInitData.push_back(pHistory->m_uiCurData);
                        iSize = pHistory->m_vuiInitData.size();
                        if(iSize > WEEK_DAYS)
                        {
                            /*ֻ����һ��7������ݣ���������ɾ��*/
                            pHistory->m_vuiInitData.erase(pHistory->m_vuiInitData.begin());
                            iSize--;
                        }
                        for(n=0; n < iSize; n++)
                        {
                            uiHistoryValue[n] = pHistory->m_vuiInitData[i];
                        }
                        for(; n < WEEK_DAYS; n++)
                        {
                            uiHistoryValue[n] = 0;
                        }
                        snprintf(szTmpData, TAXIS_HISTORY_DATA_MAX_LEN, "%llu_%u_%u",
                        pMapHistory->first, 
                        pHistory->m_uiCurData, 
                        pHistory->m_vuiInitData[0]);
                        for(n=1; n < iSize; n++)
                        {
                            snprintf(szUidata, TAXIS_MAX_UNSIGN_INT,"_%d", pHistory->m_vuiInitData[n]);
                            strncat(szTmpData, szUidata, TAXIS_MAX_UNSIGN_INT);
                        }
                        strncat(szTmpData, "\n", 1);
                        if(NULL != pFdTmp)
                        {
                            fwrite(szTmpData, 1, strlen(szTmpData), pFdTmp);
                        }
                        /*����������ע��*/
                        stToptaxisMsgTmp.m_ullNumber = (unsigned long long)pMapHistory->first;
                        stFieldInfoTmpRule.m_uiValue = pHistory->m_uiCurData;
                        stToptaxisMsgTmp.m_vFieldInfo.clear();
                        stToptaxisMsgTmp.m_vFieldInfo.push_back(stFieldInfoTmpRule);
                        stToptaxisMsgTmp.m_vConditionInfo.clear();
                        g_uiHandMsgFlag = 1;
                        g_pCTopTaxisHandle->HandleMsg(stToptaxisMsgTmp);
                        g_uiHandMsgFlag = 0;
                    }
                    if(0 != fclose(pFdTmp))
                    {
                        ERROR_LOG("closefile[fail]filename[%s]error[%d:%s]", 
                            strTmpFileName, errno, strerror(errno));
                        //continue;
                    }
                    pFdOldTmp = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_historyfd;
                    if(NULL != pFdOldTmp)
                    {
                        if(0 != fclose(pFdOldTmp))
                        {
                            ERROR_LOG("closefile[fail]filename[%s]error[%d:%s]", 
                                strPosFileName, errno, strerror(errno));	
                            //continue;
                        }
                    }
                    if(0 != unlink(strPosFileName))
                    {
                        ERROR_LOG("delfile[fail]filename[%s]error[%d:%s]", 
                            strPosFileName, errno, strerror(errno));	
                        //continue;
                    }
                    if(0 !=rename(strTmpFileName, strPosFileName))
                    {
                        ERROR_LOG("renamehistory[fail]fileorgname[%s]filedsc[%s]nameerror[%d:%s]", 
                            strTmpFileName, strPosFileName, errno, strerror(errno));
                        //continue;
                    }
                    /*���ļ�������Ϣ�����̼߳���д��������*/
                    pFdOldTmp = fopen(strPosFileName, "r+"); 
                    if(NULL == pFdOldTmp)
                    {
                        ERROR_LOG("openfail[%s]", strPosFileName);
                    }
                    if(0 !=fseek(pFdOldTmp, 0L, SEEK_END))
                    {
                        ERROR_LOG("fseek[fail]filename[%s]seekpos[SEEK_END]error[%d:%s]", 
                            strPosFileName, errno, strerror(errno));
                        /*ȷ�����ݲ��ܱ�����*/
                        continue;
                    }
                    g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_historyfd = pFdOldTmp;
                }
            }
        }
    }
    //pthread_mutex_unlock( &g_mutex_taxishistory_map);
    return 0;
}

/*************************************************************
*��������: CrotabColdDataBackup()
*�������: ��
*���ز���: ��
*��������: ÿ�춨ʱ�䱸ȫ�������ļ�
************************************************************/
int CrotabColdDataBackup(void)
{
    unsigned int uiConTmp = 0, uiRuleTmp = 0, uiTopTmp = 0;
    Sync_Toptaxis stSyncTopTaxis;

    unsigned int uiConditionIndex = 0, uiBidIndex = 0, uiTopListIndex = 0, uiRuleIndex = 0;

    char strCMD[TAXIS_MAX_CMD_LEN] = {0};
    char strColdLogFile[MAX_FILE_PATH_LEN] = {0};
    char strOKFlagFile[MAX_FILE_PATH_LEN] = {0};
    char strWarnMsg[MAX_WARN_LEN] = {0};

    struct tm curTmm;
    time_t curTime = time(NULL);

    localtime_r((time_t*)&(curTime), &curTmm);

    /*����������ɺ�ı�־�ļ�*/
    snprintf(strOKFlagFile, MAX_FILE_PATH_LEN, "%s/%s", TAXIS_COLDBAK_DATA_DIR, TAXIS_COLDBAK_OK_FLAG);

    /*�������ݵı�־�ļ������ڣ����ٽ��б���*/
    if (access (strOKFlagFile, F_OK))
    {
        WARN_LOG("ColdBak[fail]OKFLagNotExist[%s]", strOKFlagFile);
        snprintf(strWarnMsg, MAX_WARN_LEN, "Cold bak fail,OkFlag file[%s] is not exist.", strOKFlagFile);
        CWarnConf::Instance()->SendWarn(strWarnMsg);
        return ERROR_OTHER_FAILED;
    }

    snprintf(strColdLogFile, MAX_FILE_PATH_LEN, "%s/%04d%02d%02d%02d", 
    TAXIS_COLDBAK_DATA_DIR, curTmm.tm_year + 1900, curTmm.tm_mon + 1, curTmm.tm_mday, curTmm.tm_hour);
    /*�����־�ļ�*/
    snprintf(strCMD, TAXIS_MAX_CMD_LEN, "rm %s/%s", TAXIS_COLDBAK_DATA_DIR, TAXIS_COLDBAK_OK_FLAG);
    if(system(strCMD) < 0)
    {
        CRIT_LOG("reason[system_failed]cmd[%s]",strCMD);
        return -1;
    }
    /*���ͬ���ļ�*/
    if (0 == access (strColdLogFile, F_OK))
    {
        snprintf(strCMD, TAXIS_MAX_CMD_LEN, "rm %s", strColdLogFile);
        if(system(strCMD) < 0)
        {
            CRIT_LOG("reason[system_failed]cmd[%s]error[%d:%s]",strCMD, errno, strerror(errno));
            return -1;
        }
    }

    g_toptaxis_cold_waterlog.Init(strColdLogFile);

    for(uiBidIndex = 0; uiBidIndex < g_uiBidNum; uiBidIndex++)
    {
        uiConTmp = g_condition_list[uiBidIndex].m_vConditionList.size();
        for(uiConditionIndex = 0; uiConditionIndex < uiConTmp; uiConditionIndex++)
        {
            uiRuleTmp = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList.size();
            for(uiRuleIndex= 0; uiRuleIndex < uiRuleTmp; uiRuleIndex++)
            {
                uiTopTmp = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList.size();
                for(uiTopListIndex= 0; uiTopListIndex < uiTopTmp; uiTopListIndex++)
                {
                    Top_Info &r_top_info = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList[uiTopListIndex];
                    stSyncTopTaxis.m_taxispos.m_uiBidIndex = uiBidIndex;
                    stSyncTopTaxis.m_taxispos.m_uiConditionListIndex = uiConditionIndex;
                    stSyncTopTaxis.m_taxispos.m_uiRuleListIndex = uiRuleIndex;
                    stSyncTopTaxis.m_taxispos.m_uiTopListIndex = uiTopListIndex;

                    stSyncTopTaxis.m_uiConditionValueNum = r_top_info.m_vUiConditionValue.size();
                    stSyncTopTaxis.m_uiTopListNum = r_top_info.m_vKeyValueList.size();
                    /*����Ҫ���¹����ڴ�*/
                    g_pCTopTaxisHandle->WriteWaterLog(&stSyncTopTaxis, r_top_info, E_TAXIS_COLD_WATERLOG);
                }
            }
        }
    }
    g_toptaxis_cold_waterlog.CloseFile();
    /*�������ݳɹ���־�ļ�*/
    snprintf(strCMD, TAXIS_MAX_CMD_LEN, "touch %s", strOKFlagFile);
    if(system(strCMD) < 0)
    {
        CRIT_LOG("reason[system_failed]cmd[%s]",strCMD);
        return -1;
    }
    return 0;
}
/*************************************************************
*��������: RecoverData(void)
*�������: ��
*���ز���: ��
*��������: ϵͳ������ʱ��ָ�֮ǰ����������
************************************************************/
int RecoverData(void)
{
    unsigned int uiConditionIndex = 0, uiBidIndex = 0, uiRuleIndex = 0, i = 0, j = 0;
    unsigned int uiConfConditionNum = 0;
    unsigned int uiRuleNum = 0;
    unsigned int uiHistoryValue[WEEK_DAYS] = {0};

    char strTopFileName[MAX_FILE_PATH_LEN] = {0};
 
    char *pszReadStr = NULL;
    Top_Info stTopInfoTmp;
    vector<string> vTemp;
    vector<string> vKey;
    unsigned int uiSize = 0;
    int iRet = 0, iStrLen = 0;
    Toptaxis_Pos stTaxisPos;
    FILE* pFdTmp = NULL;
    All_History_Value stHistoryValueTmp;
    unsigned long long ullUinTmp = 0;
    TopTaxisShmData* pstTaxisShmData = NULL;
    int iShmID = 0, iDataTemp = 0;
    unsigned int uiHistorySize = 0;
    vector <Key_Value> v_history_key_list; 

    for(uiBidIndex = 0; uiBidIndex < g_uiBidNum; uiBidIndex++)
    {
        uiConfConditionNum = g_condition_list[uiBidIndex].m_vConditionList.size();
        /*���������ļ��е��������ҵ���Ϣ����֮ƥ�������*/
        /*�����ļ��е������б�*/
        for(uiConditionIndex = 0; uiConditionIndex < uiConfConditionNum; uiConditionIndex++)
        {
            uiRuleNum = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList.size();
            /*���������ļ�����ĳ���������й�������*/
            for(uiRuleIndex = 0; uiRuleIndex < uiRuleNum; uiRuleIndex++)
            {
                stTaxisPos.m_uiBidIndex = uiBidIndex;
                stTaxisPos.m_uiConditionListIndex = uiConditionIndex;
                stTaxisPos.m_uiRuleListIndex = uiRuleIndex;
                v_history_key_list.clear();
                while(1)
                {
                    bzero(&stTopInfoTmp, sizeof(stTopInfoTmp));
                    pszReadStr =  fgets(strTopFileName, MAX_FILE_PATH_LEN, 
                        g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_posfd);
                    /*û�����ݣ��������û�������б�*/
                    if(NULL == pszReadStr)
                    {
                        /*����һ��������ļ�*/
                        break;
                    }
                    iStrLen = strlen(strTopFileName);
                    if(iStrLen < 1)
                    {
                        /*��������һ����¼*/
                        continue;
                    }
                    vTemp.clear();
                    /* ../taxisdata/taxis_ruleid_condnum_cond1_cond2...*/
                    CStrUtil::SplitStringUsing(strTopFileName, "_", &vTemp);
                    uiSize = vTemp.size();
                    if(uiSize < TAXIS_MIN_FILED_TOPFILE)
                    {
                        ERROR_LOG("ErrorTopFile[%s]ruleid[%u]", strTopFileName,
                            g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId);
                        /*��������һ����¼*/
                        continue;
                    }
                    if(((unsigned int)atoi(vTemp[1].c_str()) != 
                        g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId)
                        ||(uiSize != (unsigned int)(atoi(vTemp[2].c_str()) + TAXIS_MIN_FILED_TOPFILE)))
                    {
                        ERROR_LOG("ErrorTopFile[%s]ruleid[%u]", strTopFileName,
                            g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId);
                        /*��������һ����¼*/
                        continue;
                    }

                    for(i = TAXIS_MIN_FILED_TOPFILE; i < uiSize; i++)
                    {
                        stTopInfoTmp.m_vUiConditionValue.push_back((unsigned int)atoi(vTemp[i].c_str()));
                    }
                    /*���һ�����з�ȥ��*/
                    strTopFileName[iStrLen - 1] = '\0'; 
                    if(access (strTopFileName, F_OK))
                    {
                        ERROR_LOG("OpenErrorTopFile[%s]", strTopFileName);
                        continue; 
                    }
                    else
                    {
                        stTopInfoTmp.m_lasttopfd = fopen(strTopFileName, "r+"); 
                        if(NULL == stTopInfoTmp.m_lasttopfd)
                        {
                            ERROR_LOG("openposfile[fail]i_fd_tmp[%s]error[%d:%s]", strTopFileName, errno, strerror(errno));
                            continue;
                        }
                    }
                    /*�����ڴ�ID*/
                    fscanf(stTopInfoTmp.m_lasttopfd, "%u\n", &(stTopInfoTmp.m_uiShmId));

                    if(0 == stTopInfoTmp.m_uiShmId)
                    {
                        ERROR_LOG("getshmid[fail]i_fd_tmp[%s]", strTopFileName);
                        continue;
                    }
                    if(stTopInfoTmp.m_uiShmId > g_ui_shm_max)
                    {
                        g_ui_shm_max = stTopInfoTmp.m_uiShmId;
                        NOTI_LOG("g_ui_shm_max[%u]", g_ui_shm_max);
                    }
                    iShmID = shmget(stTopInfoTmp.m_uiShmId, TAXIS_SHM_SIZE, 0640);
                    if( -1 == iShmID)
                    {
                        /*����־λ,bFlag Ϊflase ֱ���˳�*/
                        EMERG_LOG("reason[data cache not exit ]keyno[%u]file[%s]",stTopInfoTmp.m_uiShmId, strTopFileName);
                        /*���������ڴ�*/
                        iShmID = shmget(stTopInfoTmp.m_uiShmId, TAXIS_SHM_SIZE, 0640|IPC_CREAT|IPC_EXCL);
                        if( -1 == iShmID)
                        {
                            EMERG_LOG("reason[create shm failed]keyno[%u]error[%d]errmsg[%s]",TAXIS_SHM_SIZE,
                                errno,strerror(errno));
                            continue;
                        }
                        NOTI_LOG("create  data  cache succ !!!!!![%u]file[%s]", stTopInfoTmp.m_uiShmId, strTopFileName);

                        /*attach �����ڴ�*/
                        stTopInfoTmp.m_pDataAddr  = (void*  )shmat(iShmID, NULL, 0);
                        if((void*)-1 == stTopInfoTmp.m_pDataAddr)
                        {
                            EMERG_LOG("reason[attach shm failed]keyno[%u]error[%d]errmsg[%s]",stTopInfoTmp.m_uiShmId,
                                errno,strerror(errno));
                            continue;
                        }
                        stTaxisPos.m_uiTopListIndex = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList.size();
                        pstTaxisShmData = (TopTaxisShmData*)stTopInfoTmp.m_pDataAddr;

                        uiSize = stTopInfoTmp.m_vUiConditionValue.size();
                        pstTaxisShmData->m_usCondNum = uiSize;
                        for(i = 0; i < uiSize; i++)
                        {
                            if(i >= TAXIS_MAX_COND_NUM)
                            {
                                EMERG_LOG("CondNum[%u]OverMaxNum[%u]", uiSize, TAXIS_MAX_COND_NUM);
                                break;
                            }
                            pstTaxisShmData->m_auiCond[i] = stTopInfoTmp.m_vUiConditionValue[i];
                        }

                        pstTaxisShmData->m_stTaxisPos.m_uiBidIndex = stTaxisPos.m_uiBidIndex;
                        pstTaxisShmData->m_stTaxisPos.m_uiConditionListIndex = stTaxisPos.m_uiConditionListIndex;
                        pstTaxisShmData->m_stTaxisPos.m_uiRuleListIndex = stTaxisPos.m_uiRuleListIndex;
                        pstTaxisShmData->m_stTaxisPos.m_uiTopListIndex = stTaxisPos.m_uiTopListIndex;

                        pstTaxisShmData->m_ucABFlag = 'B';
                        pstTaxisShmData->m_uiANum = 0;
                        pstTaxisShmData->m_uiBNum = 0;
                        continue;
                    }
                    else
                    {
                        /*attach �����ڴ�*/
                        stTopInfoTmp.m_pDataAddr  = (void*  )shmat(iShmID, NULL, 0);
                        if((void*)-1 == stTopInfoTmp.m_pDataAddr )
                        {
                            EMERG_LOG("reason[attach shm failed]keyno[%u]error[%d]errmsg[%s]",stTopInfoTmp.m_uiShmId,
                                errno,strerror(errno));
                            return -1;
                        }
                        pstTaxisShmData = (TopTaxisShmData*)stTopInfoTmp.m_pDataAddr;
                        if(('A' != pstTaxisShmData->m_ucABFlag)&&('B' != pstTaxisShmData->m_ucABFlag))
                        {
                            ERROR_LOG("ErrorTopFile[%s]AB[%u]", strTopFileName, pstTaxisShmData->m_ucABFlag);
                            continue;
                        }
                        stTaxisPos.m_uiTopListIndex = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList.size();

                        if('A' == pstTaxisShmData->m_ucABFlag)
                        {
                            if(E_REAL_DATA != g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_usTaxisTime)
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiANum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tAData[i]);
                                    v_history_key_list.push_back(pstTaxisShmData->m_tAData[i]);
                                    /*��ѯServer����Ҫ��������*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*ȫ���û�����������ҲҪ����һ������*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tAData[i].m_ullKey, stTaxisPos);
                                }
                            }
                            else
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiANum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tAData[i]);
                                    /*��ѯServer����Ҫ��������*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*ȫ���û�����������ҲҪ����һ������*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tAData[i].m_ullKey, stTaxisPos);
                                }
                            }
                        }
                        if('B' == pstTaxisShmData->m_ucABFlag)
                        {
                            if(E_REAL_DATA != g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_usTaxisTime)
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiBNum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tBData[i]);
                                    v_history_key_list.push_back(pstTaxisShmData->m_tBData[i]);
                                    /*��ѯServer����Ҫ��������*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*ȫ���û�����������ҲҪ����һ������*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tBData[i].m_ullKey, stTaxisPos);
                                }
                            }
                            else
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiBNum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tBData[i]);
                                    /*��ѯServer����Ҫ��������*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*ȫ���û�����������ҲҪ����һ������*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tBData[i].m_ullKey, stTaxisPos);
                                }
                            }
                        }
                        g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList.push_back(stTopInfoTmp);
                    }
                }
                /*��ѯServer����Ҫ�ָ���ʷ����*/
                if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                {
                    continue;
                }
                /*����ʷ�������ƹ�Ʊ�Ƿ�������ԭʼֵ(ǰһ�죬ǰһ��)*/
                pFdTmp = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_historyfd;
                MAP_HISTORY_VALUE &r_map_history = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_mapHostoryList;
                if((E_DAY_DATA == g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_usTaxisTime)
                    &&(NULL != pFdTmp))
                {
                    while(1)
                    {
                        bzero(&stHistoryValueTmp, sizeof(stHistoryValueTmp));
                        /*Uin_curdata_history1_history2_\n*/
                        iRet = fscanf(pFdTmp, "%llu_%u_%u\n", 
                            &ullUinTmp,
                            &(stHistoryValueTmp.m_uiCurData),
                            &uiHistoryValue[0]);
                        if((iRet < 0)||(TAXIS_HISTORY_DATA_NUM_DAY != iRet))
                        {
                            break;
                        }
                        stHistoryValueTmp.m_vuiInitData.push_back(uiHistoryValue[0]);
                        r_map_history.insert(make_pair(ullUinTmp, stHistoryValueTmp));
                    }
                    uiHistorySize = v_history_key_list.size();
                    for(j = 0; j < uiHistorySize; j++)
                    {
                        g_pCTopTaxisHandle->UpdateCurData(r_map_history,
                            v_history_key_list[j].m_iValue,
                            v_history_key_list[j].m_ullKey,
                            &iDataTemp,
                            g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_historyfd,
                            E_DAY_DATA);
                    }
                }
                if((E_WEEK_DATA == g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_usTaxisTime)
                &&(NULL != pFdTmp))
                {
                    while(1)
                    {
                        bzero(&stHistoryValueTmp, sizeof(stHistoryValueTmp));
                        /*Uin_curdata_history1_history2_\n*/
                        iRet = fscanf(pFdTmp, "%llu_%u_%u_%u_%u_%u_%u_%u_%u\n", 
                            &ullUinTmp,
                            &(stHistoryValueTmp.m_uiCurData),
                            &uiHistoryValue[0], &uiHistoryValue[1],&uiHistoryValue[2],
                            &uiHistoryValue[3],&uiHistoryValue[4],&uiHistoryValue[5], &uiHistoryValue[6]);
                        /*finish*/
                        if(iRet <= 0)
                        {
                            break;
                        }
                        if((iRet > TAXIS_HISTORY_DATA_NUM_WEEK)||(iRet < TAXIS_HISTORY_DATA_NUM_DAY))
                        {
                            ERROR_LOG("ReadHistoryFile[overnum]num[%d]ruleid[%u]", iRet,
                                g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId);
                            continue;
                        }
                        iRet = iRet - TAXIS_HISTORY_DATA_NUM_DAY + 1;
                        for(i = 0; i < (unsigned int)iRet; i++)
                        {
                            stHistoryValueTmp.m_vuiInitData.push_back(uiHistoryValue[i]);
                        }
                        r_map_history.insert(make_pair(ullUinTmp, stHistoryValueTmp));
                    }
                    uiHistorySize = v_history_key_list.size();
                    for(j = 0; j < uiHistorySize; j++)
                    {
                        g_pCTopTaxisHandle->UpdateCurData(r_map_history,
                            v_history_key_list[j].m_iValue,
                            v_history_key_list[j].m_ullKey,
                            &iDataTemp,
                            g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_historyfd,
                            E_WEEK_DATA);
                    }
                }
            }
        }
    }
    return 0;
}
