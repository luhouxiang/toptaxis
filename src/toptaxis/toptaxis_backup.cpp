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
*      Description:   TOP排序系统系统数据备份恢复
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

/*系统中所有条件列表*/
extern vector <Bid_Condition> g_condition_list;
/*系统中所有条件列表*/
extern map<unsigned int, set<string> > g_cond_list; 
/*系统中所有规则列表*/
extern map<unsigned int, set<string> > g_field_list; 
/*系统中业务数目*/
extern unsigned int g_uiBidNum; 
extern pthread_mutex_t g_mutex_taxishistory_map;

/*记录用户在排行榜的位置*/
extern MAP_TAXIS_POS g_user_taxis_pos_list;

/*共享内存(多进程)操作的锁*/
extern CCommSemp g_cCommSemp;

/*记录流水数据的缓冲*/
extern char* g_sz_waterlogdata;

/*通知Notify同步排序数据到备机*/
extern CNotifySync g_cNotifySync;

/*系统使用共享内存的起始Key*/
extern unsigned int g_ui_shm_max;

/*全量流水*/
extern CColdLog  g_toptaxis_cold_waterlog;

/*全局配置类*/
extern CTaxisServiceConf*  g_pCServiceConf;

/*全局处理排序消息的对象*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

/*当前是否在处理消息标志, 便于每个消息只记录一次流水*/
extern unsigned int g_uiHandMsgFlag;

CColdLog::CColdLog() 
{
    memset(m_szLogPath, 0, sizeof(m_szLogPath));
    m_iLogfd = ERR_FD_NO;
}
/*************************************************************
*函数名称: Init()
*输入参数: szlogpath:写冷备流水文件的完整路径
*返回参数:     0 : 成功
*          其它值: 失败
*函数功能: 写当前的所有排序数据到磁盘文件
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
*函数名称: WriteLog()
*输入参数: data    :冷备流水的数据
*          datalen :冷备流水的数据长度
*返回参数: 大于等于0 : 写文件的数据个数
*          其它值: 失败
*函数功能: 写当前的所有排序数据到磁盘文件
************************************************************/
int CColdLog::WriteLog(const char * data, unsigned int datalen)
{
    int retLen = 0;

    /* check fd*/
    /* 上次写失败的时候 fd 将置空*/
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
    /* 返回一共写入的数据长度*/
    return retLen; 
}
/*************************************************************
*函数名称: CloseFile()
*输入参数: 略
*返回参数: 略
*函数功能: 关闭冷备文件
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
*函数名称: ~CColdLog()
*输入参数: 略
*返回参数: 略
*函数功能: 释放资源
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
*函数名称: CrotabUpdateHistoryData()
*输入参数: 略
*返回参数: 略
*函数功能: 更新当天的初始值
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
                    /*先把此规则排序的数据数据清空*/
                    /*不需要清空，依赖后面的week的hand_msg消息进行清空处理*/
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
                        /*更新初始值为当天变化的最终值*/
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
                    /*打开文件方便消息处理线程继续写增量数据*/
                    pFdOldTmp = fopen(strPosFileName, "r+"); 
                    if(NULL == pFdOldTmp)
                    {
                        ERROR_LOG("openfail[%s]", strPosFileName);
                    }
                    if(0 !=fseek(pFdOldTmp, 0L, SEEK_END))
                    {
                        ERROR_LOG("fseek[fail]filename[%s]seekpos[SEEK_END]error[%d:%s]\n", 
                        strPosFileName, errno, strerror(errno));
                        /*确保数据不能被覆盖*/
                        continue;
                    }
                    g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_historyfd = pFdOldTmp;
                }
            }
            else if(E_WEEK_DATA == g_condition_list[i].m_vConditionList[j].m_usTaxisTime)
            {
                /*把数据重新注入*/
                stToptaxisMsgTmp.m_uiBid = g_condition_list[i].m_uiBid;
                stToptaxisMsgTmp.m_usCondChange = 0;

                uiRuleTmp = g_condition_list[i].m_vConditionList[j].m_vRuleList.size();
                for(k = 0; k < uiRuleTmp; k++)
                {
                    /*先把此规则排序的数据数据清空*/
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
                    /*把数据重新注入*/
                    stFieldInfoTmpRule.m_sField = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_sFieldId;

                    for(pMapHistory = g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.begin(); 
                        pMapHistory != g_condition_list[i].m_vConditionList[j].m_vRuleList[k].m_mapHostoryList.end(); 
                        pMapHistory++)
                    {
                        pHistory = (All_History_Value*)&(pMapHistory->second);
                        /*更新初始值为当天变化的最终值*/
                        pHistory->m_vuiInitData.push_back(pHistory->m_uiCurData);
                        iSize = pHistory->m_vuiInitData.size();
                        if(iSize > WEEK_DAYS)
                        {
                            /*只保存一周7天的数据，过期数据删除*/
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
                        /*把数据重新注入*/
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
                    /*打开文件方便消息处理线程继续写增量数据*/
                    pFdOldTmp = fopen(strPosFileName, "r+"); 
                    if(NULL == pFdOldTmp)
                    {
                        ERROR_LOG("openfail[%s]", strPosFileName);
                    }
                    if(0 !=fseek(pFdOldTmp, 0L, SEEK_END))
                    {
                        ERROR_LOG("fseek[fail]filename[%s]seekpos[SEEK_END]error[%d:%s]", 
                            strPosFileName, errno, strerror(errno));
                        /*确保数据不能被覆盖*/
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
*函数名称: CrotabColdDataBackup()
*输入参数: 略
*返回参数: 略
*函数功能: 每天定时冷备全量排序文件
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

    /*备份数据完成后的标志文件*/
    snprintf(strOKFlagFile, MAX_FILE_PATH_LEN, "%s/%s", TAXIS_COLDBAK_DATA_DIR, TAXIS_COLDBAK_OK_FLAG);

    /*备份数据的标志文件不存在，则不再进行备份*/
    if (access (strOKFlagFile, F_OK))
    {
        WARN_LOG("ColdBak[fail]OKFLagNotExist[%s]", strOKFlagFile);
        snprintf(strWarnMsg, MAX_WARN_LEN, "Cold bak fail,OkFlag file[%s] is not exist.", strOKFlagFile);
        CWarnConf::Instance()->SendWarn(strWarnMsg);
        return ERROR_OTHER_FAILED;
    }

    snprintf(strColdLogFile, MAX_FILE_PATH_LEN, "%s/%04d%02d%02d%02d", 
    TAXIS_COLDBAK_DATA_DIR, curTmm.tm_year + 1900, curTmm.tm_mon + 1, curTmm.tm_mday, curTmm.tm_hour);
    /*清除标志文件*/
    snprintf(strCMD, TAXIS_MAX_CMD_LEN, "rm %s/%s", TAXIS_COLDBAK_DATA_DIR, TAXIS_COLDBAK_OK_FLAG);
    if(system(strCMD) < 0)
    {
        CRIT_LOG("reason[system_failed]cmd[%s]",strCMD);
        return -1;
    }
    /*清除同名文件*/
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
                    /*不需要更新共享内存*/
                    g_pCTopTaxisHandle->WriteWaterLog(&stSyncTopTaxis, r_top_info, E_TAXIS_COLD_WATERLOG);
                }
            }
        }
    }
    g_toptaxis_cold_waterlog.CloseFile();
    /*创建备份成功标志文件*/
    snprintf(strCMD, TAXIS_MAX_CMD_LEN, "touch %s", strOKFlagFile);
    if(system(strCMD) < 0)
    {
        CRIT_LOG("reason[system_failed]cmd[%s]",strCMD);
        return -1;
    }
    return 0;
}
/*************************************************************
*函数名称: RecoverData(void)
*输入参数: 略
*返回参数: 略
*函数功能: 系统重启的时候恢复之前的排序数据
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
        /*遍历规则文件中的条件，找到消息中与之匹配的条件*/
        /*规则文件中的条件列表*/
        for(uiConditionIndex = 0; uiConditionIndex < uiConfConditionNum; uiConditionIndex++)
        {
            uiRuleNum = g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList.size();
            /*遍历规则文件里面某条件的所有规则属性*/
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
                    /*没有数据，则表明还没有排序列表*/
                    if(NULL == pszReadStr)
                    {
                        /*读下一条规则的文件*/
                        break;
                    }
                    iStrLen = strlen(strTopFileName);
                    if(iStrLen < 1)
                    {
                        /*继续读下一条记录*/
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
                        /*继续读下一条记录*/
                        continue;
                    }
                    if(((unsigned int)atoi(vTemp[1].c_str()) != 
                        g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId)
                        ||(uiSize != (unsigned int)(atoi(vTemp[2].c_str()) + TAXIS_MIN_FILED_TOPFILE)))
                    {
                        ERROR_LOG("ErrorTopFile[%s]ruleid[%u]", strTopFileName,
                            g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_uiRuleId);
                        /*继续读下一条记录*/
                        continue;
                    }

                    for(i = TAXIS_MIN_FILED_TOPFILE; i < uiSize; i++)
                    {
                        stTopInfoTmp.m_vUiConditionValue.push_back((unsigned int)atoi(vTemp[i].c_str()));
                    }
                    /*最后一个换行符去掉*/
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
                    /*共享内存ID*/
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
                        /*检查标志位,bFlag 为flase 直接退出*/
                        EMERG_LOG("reason[data cache not exit ]keyno[%u]file[%s]",stTopInfoTmp.m_uiShmId, strTopFileName);
                        /*创建共享内存*/
                        iShmID = shmget(stTopInfoTmp.m_uiShmId, TAXIS_SHM_SIZE, 0640|IPC_CREAT|IPC_EXCL);
                        if( -1 == iShmID)
                        {
                            EMERG_LOG("reason[create shm failed]keyno[%u]error[%d]errmsg[%s]",TAXIS_SHM_SIZE,
                                errno,strerror(errno));
                            continue;
                        }
                        NOTI_LOG("create  data  cache succ !!!!!![%u]file[%s]", stTopInfoTmp.m_uiShmId, strTopFileName);

                        /*attach 共享内存*/
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
                        /*attach 共享内存*/
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
                                    /*查询Server不需要更新数据*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*全局用户排序链表中也要插入一组数据*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tAData[i].m_ullKey, stTaxisPos);
                                }
                            }
                            else
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiANum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tAData[i]);
                                    /*查询Server不需要更新数据*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*全局用户排序链表中也要插入一组数据*/
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
                                    /*查询Server不需要更新数据*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*全局用户排序链表中也要插入一组数据*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tBData[i].m_ullKey, stTaxisPos);
                                }
                            }
                            else
                            {
                                for(i = 0; i < pstTaxisShmData->m_uiBNum; i++)
                                {
                                    stTopInfoTmp.m_vKeyValueList.push_back(pstTaxisShmData->m_tBData[i]);
                                    /*查询Server不需要更新数据*/
                                    if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                                    {
                                        continue;
                                    }
                                    /*全局用户排序链表中也要插入一组数据*/
                                    g_pCTopTaxisHandle->InsertTaxisPosNode(pstTaxisShmData->m_tBData[i].m_ullKey, stTaxisPos);
                                }
                            }
                        }
                        g_condition_list[uiBidIndex].m_vConditionList[uiConditionIndex].m_vRuleList[uiRuleIndex].m_vTopList.push_back(stTopInfoTmp);
                    }
                }
                /*查询Server不需要恢复历史数据*/
                if(E_TAXIS_QUERY_SERVER == g_pCServiceConf->m_uiServerType)
                {
                    continue;
                }
                /*读历史数据类似股票涨幅跌幅的原始值(前一天，前一周)*/
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
