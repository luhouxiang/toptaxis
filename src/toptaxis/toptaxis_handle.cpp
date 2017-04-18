/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_handle.cpp
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description:   对系统输入的数据进行处理，形成Top排序数据
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

#include <dirent.h> 

#include "strutil.h"
#include "cache_manage.h"
#include "poll_thread.h"
#include "listener.h"
#include "client_sync.h"
#include "log.h"
#include "timestamp.h"
#include "ts_waterlog.h"
#include "comm_semp.h"
#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "toptaxis_handle.h"
#include "toptaxis_sync.h"
#include "toptaxis_backup.h"
#include "toptaxis_conf.h"

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include<sched.h> 
#include<ctype.h> 
#include <map>

#include "dc_process.h"
#include "send_warn.h"

using namespace std;
extern vector <Bid_Condition> g_condition_list;/*系统中所有条件列表*/

extern vector <Bid_Condition> gReloadCondList;

extern map<unsigned int, set<string> > g_cond_list; /*系统中所有条件列表*/
extern map<unsigned int, set<string> > g_field_list; /*系统中所有条件列表*/
extern unsigned int g_uiBidNum; /*系统中业务数目*/
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

/*全局处理排序消息的对象*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

/*系统中所有的规则*/
extern MAP_TAXIS_RULE_LIST gMapRuleList;

/*规则配置类*/
extern CRuleConf* g_pCRuleConf;

map<string, unsigned int> g_mFileLastChange;

unsigned int g_uiRuleFileNumPos;

/*全局规则配置文件与Bid的关系映射表*/
extern MAP_TAXIS_RULECONF_BID g_MapRuleConfBid;

extern CTaxisServiceConf * g_pCServiceConf;

/*当前是否在处理消息标志, 便于每个消息只记录一次流水*/
extern unsigned int g_uiHandMsgFlag;

/*************************************************************
*函数名称: FindWhiteList()
*输入参数: 略
*返回参数: 略
*函数功能: 判断是否在白名单之中
************************************************************/
int CTopTaxisHandle::FindWhiteList(V_WHITE_LIST &v_whitelist, unsigned long long ullIn)
{
    unsigned int uiSize = 0, i = 0;
    uiSize = v_whitelist.size();

    for(i = 0; i < uiSize; i++)
    {
        if(ullIn == v_whitelist[i])
        {
            return 0;
        }
    }
    return -1;    
}

/*************************************************************
*函数名称: vector_undata_sort()
*输入参数: 略
*返回参数: 略
*函数功能: 对满足条件规则的数据进行排序
************************************************************/
int CTopTaxisHandle::VectorUndataSort(Toptaxis_Pos &r_toptaxis_pos, int iCurData, int uiData, unsigned long long ullUin,
    unsigned int uiSortNum, unsigned short usTaixType, unsigned long long &ullPopUin, Sort_Pos_Info &r_posinfo)
{
    int i_vector_len = 0, i_first_pos = 0, i_last_pos = 0;
    int i_old_value = 0, i_old_change = 0;
    Key_Value key_value_temp; 
    int i_exist_toptaix = 0, i_insert_flag = 0;
    unsigned short us_type = 0;

    key_value_temp.m_ullKey = ullUin;
    key_value_temp.m_iValue = iCurData;
    key_value_temp.m_iChangeValue = uiData;

    r_posinfo.m_i_firstpos =  0;
    r_posinfo.m_i_lastpos =  0;
    Bid_Condition& rstBidTmp = g_condition_list[r_toptaxis_pos.m_uiBidIndex];
    Condition_Info& rstCondTmp = rstBidTmp.m_vConditionList[r_toptaxis_pos.m_uiConditionListIndex];
    Rule_Info& rstRuleTmp = rstCondTmp.m_vRuleList[r_toptaxis_pos.m_uiRuleListIndex];
    Top_Info& rstTopTmp = rstRuleTmp.m_vTopList[r_toptaxis_pos.m_uiTopListIndex];

    us_type = rstCondTmp.m_usTaxisTime;
    if(E_REAL_DATA == us_type)
    {
        key_value_temp.m_iChangeValue = 0;
    }
    
    vector<Key_Value> &v_ListOrg = rstTopTmp.m_vKeyValueList;
    i_vector_len = v_ListOrg.size();
    /*如果排序列表中已有该用户的数据，先摘除掉后再排序*/
    for(i_first_pos = 0; i_first_pos < i_vector_len; i_first_pos++)
    {
        if(v_ListOrg[i_first_pos].m_ullKey == ullUin)
        {
            i_old_value = v_ListOrg[i_first_pos].m_iValue;
            i_old_change = v_ListOrg[i_first_pos].m_iChangeValue;
            /*如果参数值没有变化，则不进行排序*/
            if(((i_old_value == iCurData)&&(E_REAL_DATA == us_type))
                ||((i_old_change == uiData)&&(E_REAL_DATA != us_type)))
            {
                if(E_REAL_DATA != us_type)
                {
                    /*股票的当前值有变化，也需要更新*/
                    if(iCurData != i_old_value)
                    {
                        v_ListOrg[i_first_pos].m_iValue = iCurData;
                        ullPopUin = 0;
                        return 0;
                    }
                }
                DEBUG_LOG("Uin[%llu]Value[%d]Change[%d]Code[NoChange]", ullUin, iCurData, uiData);
                return -1;
            }
            v_ListOrg.erase(v_ListOrg.begin()+i_first_pos);
            /*在排序链表中纯在，后面即使再插入也不用在用户排序链表中记录*/
            i_exist_toptaix = 1;
            i_vector_len--;
            break;
        }
    }
    r_posinfo.m_i_firstpos =  i_first_pos;
    DEBUG_LOG("Uin[%llu]Value[%d]Code[Push]", ullUin, uiData);
    /*降序排列，最小值排在最前面*/
    if(E_TOP_DOWN == usTaixType)
    {
        /*如果是普通排序，则只对当前值排序*/
        if(E_REAL_DATA == us_type)
        {
            for(i_last_pos = 0; i_last_pos < i_vector_len; i_last_pos++)
            {
                if(v_ListOrg[i_last_pos].m_iValue > iCurData)
                {
                    v_ListOrg.insert(v_ListOrg.begin()+i_last_pos, key_value_temp);
                    i_insert_flag = 1;
                    if(0 == i_exist_toptaix)
                    {
                        g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
                    }
                    break;
                }
            }
        }
        else/*如果是涨跌幅排序，则对改变值排序*/
        {
            for(i_last_pos = 0; i_last_pos < i_vector_len; i_last_pos++)
            {
                if(v_ListOrg[i_last_pos].m_iChangeValue > uiData)
                {
                    v_ListOrg.insert(v_ListOrg.begin()+i_last_pos, key_value_temp);
                    i_insert_flag = 1;
                    if(0 == i_exist_toptaix)
                    {
                        g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
                    }
                    break;
                }
            }
        }
        /*如果是最大值，则插在最后*/
        if(i_last_pos == i_vector_len)
        {
            if(i_vector_len >= (int)uiSortNum)
            {
                /*如果有删除，则说明排序表有变化，需要写流水*/
                if(1 == i_exist_toptaix)
                {
                    r_posinfo.m_i_lastpos =  i_first_pos;
                    return 0;
                }
                /*最大值则溢出不处理*/
                return -1;
            }
            i_insert_flag = 1;
            v_ListOrg.push_back(key_value_temp);
            if(0 == i_exist_toptaix)
            {
                g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
            }
        }
    }
    else
    {
        /*如果是普通排序，则只对当前值排序*/
        if(E_REAL_DATA == us_type)
        {
            for(i_last_pos = 0; i_last_pos < i_vector_len; i_last_pos++)
            {
                if(v_ListOrg[i_last_pos].m_iValue < iCurData)
                {
                    v_ListOrg.insert(v_ListOrg.begin()+i_last_pos, key_value_temp);
                    i_insert_flag = 1;
                    if(0 == i_exist_toptaix)
                    {
                        g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
                    }
                    break;
                }
            }
        }
        else/*如果是涨跌幅排序，则对改变值排序*/
        {
            for(i_last_pos = 0; i_last_pos < i_vector_len; i_last_pos++)
            {
                if(v_ListOrg[i_last_pos].m_iChangeValue < uiData)
                {
                    v_ListOrg.insert(v_ListOrg.begin()+i_last_pos, key_value_temp);
                    i_insert_flag = 1;
                    if(0 == i_exist_toptaix)
                    {
                        g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
                    }
                    break;
                }
            }
        }
        /*如果是最小值，则插在最后*/
        if(i_last_pos == i_vector_len)
        {
            if(i_vector_len >= (int)uiSortNum)
            {
                /*如果有删除，则说明排序表有变化，需要写流水*/
                if(1 == i_exist_toptaix)
                {
                    r_posinfo.m_i_lastpos =  i_first_pos;
                    return 0;
                }
                /*最小值则溢出不处理*/
                return -1;
            }
            i_insert_flag = 1;
            v_ListOrg.push_back(key_value_temp);
            if(0 == i_exist_toptaix)
            {
                g_pCTopTaxisHandle->InsertTaxisPosNode(ullUin, r_toptaxis_pos);
            }
        }
    }
    r_posinfo.m_i_lastpos =  i_last_pos;
    /*重新获取新排序链表的节点数目*/
    i_vector_len = v_ListOrg.size();
    /*超出排序节点总数，则删除最后一个节点*/
    if((i_vector_len > (int)uiSortNum)&&(i_vector_len > 0))
    {
        ullPopUin = v_ListOrg[i_vector_len-1].m_ullKey;
        v_ListOrg.pop_back();
    }
    return 0;
}

/*************************************************************
*函数名称: vector_undata_compare()
*输入参数: 略
*返回参数: 略
*函数功能: 比较2个vector的数据是否完全相等
************************************************************/
int CTopTaxisHandle::VectorUndataCompare(vector<unsigned int> &v_uiListOrg, vector<unsigned int> &v_uiListDst)
{
    unsigned int i_vector_len = 0, i = 0;

    i_vector_len = v_uiListOrg.size();
    if(i_vector_len != v_uiListDst.size())
    {
        DEBUG_LOG("ListOrgSize[%u]ListDstSize[%u]", i_vector_len, v_uiListDst.size());
        return -1;
    }
    for(i =0; i < i_vector_len; i++)
    {
        if(v_uiListOrg[i] != v_uiListDst[i])
        {
            DEBUG_LOG("Index[%u]ListOrg[%u]ListDst[%u]", i, v_uiListOrg[i], v_uiListDst[i]);
            return -1;
        }
    }
    return 0;
}

/*************************************************************
*函数名称: update_cur_data()
*输入参数: 略
*返回参数: 略
*函数功能: 更新需要保存历史数据的当前值
************************************************************/
int CTopTaxisHandle::UpdateCurData(MAP_HISTORY_VALUE& map_hostory, unsigned int uiValue, unsigned long long ullUin, 
    int* iDiffData, FILE* pFd, unsigned short usTimeType)
{
    MAP_HISTORY_VALUE::iterator p_map_history;
    All_History_Value* p_history = NULL;
    All_History_Value t_history_temp;
    char sz_buf[TAXIS_HISTORY_DATA_MAX_LEN] = {0};
    if(NULL == iDiffData)
    {
        ERROR_LOG("iDiffData[NULL]");
        return -1;
    }
    p_map_history = map_hostory.find(ullUin);
    if(p_map_history != map_hostory.end())
    {
        p_history = (All_History_Value*)&(p_map_history->second);
            p_history->m_uiCurData = uiValue;
            if(0 == p_history->m_vuiInitData[0])
            {
                *iDiffData = 0;
            }
            else
            {
                *iDiffData = (int)((((int)(p_history->m_uiCurData - p_history->m_vuiInitData[0]))*TAXIS_CHANGE_DATA_RATE)/p_history->m_vuiInitData[0]);
            }
            return 0;
    }
    /*新接点则需要设置初始值和当前值*/
    t_history_temp.m_vuiInitData.push_back(uiValue);
    t_history_temp.m_uiCurData = uiValue;
    map_hostory.insert(make_pair(ullUin, t_history_temp));
    *iDiffData = 0;
    /*新节点则需要在历史文件中插入记录*/
    snprintf(sz_buf, TAXIS_HISTORY_DATA_MAX_LEN, "%llu_%u_%u\n", ullUin, uiValue, uiValue);
    if((NULL == pFd)||((E_WEEK_DATA!=usTimeType)&&(E_DAY_DATA!=usTimeType)))
    {
        ERROR_LOG("pFd[NULL]usTimeType[%u]data[%s]", usTimeType, sz_buf);       
        return -1;
    }
    fwrite(sz_buf, 1, strlen(sz_buf), pFd);
    return 0;
}
/*************************************************************
*函数名称: del_taxis_top_list_node()
*输入参数: 略
*返回参数: 略
*函数功能: 删除排序链表里面的节点
************************************************************/
int CTopTaxisHandle::DelTaxisTopListNode(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos)
{
    unsigned int uiBidSize = 0, uiCondSize = 0, uiRuleSize = 0, uiTopSize = 0;

    uiBidSize = g_condition_list.size();
    if(taxispos.m_uiBidIndex >= uiBidSize)
    {
        ERROR_LOG("OverLoad[!!!!!]BidSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]", 
            uiBidSize, 
            taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex, 
            taxispos.m_uiRuleListIndex, taxispos.m_uiTopListIndex);
        return 0;
    }
    Bid_Condition& rstBidTmp = g_condition_list[taxispos.m_uiBidIndex];
    uiCondSize = rstBidTmp.m_vConditionList.size();
    if(taxispos.m_uiConditionListIndex >= uiCondSize)
    {
        ERROR_LOG("OverLoad[!!!!!]CondSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]",
            uiCondSize,
            taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex, 
            taxispos.m_uiRuleListIndex, taxispos.m_uiTopListIndex);
        return 0;
    }
    Condition_Info& rstCondTmp = rstBidTmp.m_vConditionList[taxispos.m_uiConditionListIndex];
    uiRuleSize = rstCondTmp.m_vRuleList.size();
    if(taxispos.m_uiRuleListIndex >= uiRuleSize)
    {
        ERROR_LOG("OverLoad[!!!!!]RuleSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]", 
            uiRuleSize, 
            taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex, 
            taxispos.m_uiRuleListIndex, taxispos.m_uiTopListIndex);
        return 0;
    }
    Rule_Info& rstRuleTmp = rstCondTmp.m_vRuleList[taxispos.m_uiRuleListIndex];
    uiTopSize = rstRuleTmp.m_vTopList.size();
    if(taxispos.m_uiTopListIndex >= uiTopSize)
    {
        ERROR_LOG("OverLoad[!!!!!]TopSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]", 
            uiTopSize, 
            taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex, 
            taxispos.m_uiRuleListIndex, taxispos.m_uiTopListIndex);
        return 0;
    }
    Top_Info& rstTopTmp = rstRuleTmp.m_vTopList[taxispos.m_uiTopListIndex];

    vector<Key_Value> &vListOrg = rstTopTmp.m_vKeyValueList;
    
    unsigned int uiVSize = 0, i =0;
    Sync_Toptaxis stSyncTopTaxis;
    uiVSize = vListOrg.size();
    
    stSyncTopTaxis.m_taxispos.m_uiBidIndex = taxispos.m_uiBidIndex;
    stSyncTopTaxis.m_taxispos.m_uiConditionListIndex = taxispos.m_uiConditionListIndex;
    stSyncTopTaxis.m_taxispos.m_uiRuleListIndex = taxispos.m_uiRuleListIndex;
    stSyncTopTaxis.m_taxispos.m_uiTopListIndex = taxispos.m_uiTopListIndex;

    /*便于写流水*/
    stSyncTopTaxis.m_uiConditionValueNum = rstTopTmp.m_vUiConditionValue.size();

    for(i =0; i < uiVSize; i++)
    {
        if(ullDelPopUin == vListOrg[i].m_ullKey)
        {
            vListOrg.erase(vListOrg.begin()+i);
            DEBUG_LOG("DelTaxisPosList[%llu]BidIndex[%u]ConditionListIndex[%u]RuleListIndex[%u]TopListIndex[%u]Pos[%u]", 
                ullDelPopUin, taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex, 
                taxispos.m_uiRuleListIndex, taxispos.m_uiTopListIndex, i);
            stSyncTopTaxis.m_uiTopListNum = vListOrg.size();
            if(stSyncTopTaxis.m_uiTopListNum >  rstRuleTmp.m_uiTopDisplayNum)
            {
                stSyncTopTaxis.m_uiTopListNum = rstRuleTmp.m_uiTopDisplayNum;
            }

            /*更新共享内存同步到备机*/
            WriteWaterLog(&stSyncTopTaxis,
                rstRuleTmp.m_vTopList[taxispos.m_uiTopListIndex],
                E_TAXIS_REAL_WATERLOG);
            return 0;
        }
    }
    return -1;
}
/*************************************************************
*函数名称: DelUinAllTaxis()
*输入参数: 略
*返回参数: 略
*函数功能: 删除用户的所有的排序数据
************************************************************/
int CTopTaxisHandle::DelUinAllTaxis(unsigned long long ullDelPopUin, unsigned int uiBidIndex)
{
    MAP_TAXIS_POS::iterator pMapTaxisPos;
    unsigned int uiVSize = 0;
    int i = 0;
    Toptaxis_Pos *pTopTaxisPos = NULL;
    pMapTaxisPos = g_user_taxis_pos_list.find(ullDelPopUin);
    if(pMapTaxisPos == g_user_taxis_pos_list.end())
    {
        DEBUG_LOG("UserTaxisPosList[%llu]Result[NoData]", ullDelPopUin);
        return 0;
    }
    /*找到存在用户的所有排序列表*/
    V_Toptaxis_Pos &vTaxisPosTmp = pMapTaxisPos->second;
    uiVSize = vTaxisPosTmp.size();
    for(i = 0; i < (int)uiVSize; i++)
    {
        pTopTaxisPos = (Toptaxis_Pos*)(&vTaxisPosTmp[i]);
        if(NULL == pTopTaxisPos)
        {
            DEBUG_LOG("pTopTaxisPos[NULL]Index[%d]Uin[%llu]BidIndex[%u]", i, ullDelPopUin, uiBidIndex);
            continue;
        }
        /*只删除本业务的信息*/
        if(uiBidIndex == pTopTaxisPos->m_uiBidIndex)
        {
            DEBUG_LOG("UserTaxisPosList[%llu]Result[%d]", ullDelPopUin, i);
            DelTaxisTopListNode(ullDelPopUin, *pTopTaxisPos);
        }
    }
    /*在用户排序列表里面删除该号码的数据*/
    g_user_taxis_pos_list.erase(pMapTaxisPos);
    return 0;

}
/*************************************************************
*函数名称: del_taxis_pos_rule()
*输入参数: 略
*返回参数: 略
*函数功能: 条件列表中是否存在用户数据
************************************************************/
int CTopTaxisHandle::DelTaxisPosRule(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos)
{
    MAP_TAXIS_POS::iterator p_map_taxispos;
    unsigned int ui_v_size = 0;
    int i = 0;
    Toptaxis_Pos *p_toptaxis_pos = NULL;
    p_map_taxispos = g_user_taxis_pos_list.find(ullDelPopUin);
    if(p_map_taxispos == g_user_taxis_pos_list.end())
    {
        return -1;
    }
    /*找到存在用户的所有排序列表*/
    V_Toptaxis_Pos &v_taxis_pos_tmp = p_map_taxispos->second;
    ui_v_size = v_taxis_pos_tmp.size();
    for(i = 0; i < (int)ui_v_size; i++)
    {
        if((taxispos.m_uiBidIndex == v_taxis_pos_tmp[i].m_uiBidIndex)&&
        (taxispos.m_uiConditionListIndex == v_taxis_pos_tmp[i].m_uiConditionListIndex))
        {
            p_toptaxis_pos = (Toptaxis_Pos*)(&v_taxis_pos_tmp[i]);
            DelTaxisTopListNode(ullDelPopUin, *p_toptaxis_pos);
            v_taxis_pos_tmp.erase(v_taxis_pos_tmp.begin()+i);
            i--;
            ui_v_size--;
            continue;
        }
    }
    /*如果用户排序链表中已经没有数据，则删除*/
    if(ui_v_size <= 0)
    {
        g_user_taxis_pos_list.erase(p_map_taxispos);
    }
    return -1;

}
/*************************************************************
*函数名称: del_taxis_pos_node()
*输入参数: 略
*返回参数: 略
*函数功能: 删除排序链表里面的节点
************************************************************/
int CTopTaxisHandle::DelTaxisPosNode(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos)
{
    MAP_TAXIS_POS::iterator p_map_taxispos;
    unsigned int uiVSize = 0, i = 0;
    p_map_taxispos = g_user_taxis_pos_list.find(ullDelPopUin);
    if(p_map_taxispos == g_user_taxis_pos_list.end())
    {
        return -1;
    }
    V_Toptaxis_Pos &v_taxis_pos_tmp = p_map_taxispos->second;
    uiVSize = v_taxis_pos_tmp.size();
    for(i = 0; i < uiVSize; i++)
    {
        if((taxispos.m_uiBidIndex == v_taxis_pos_tmp[i].m_uiBidIndex)&&
        (taxispos.m_uiConditionListIndex == v_taxis_pos_tmp[i].m_uiConditionListIndex)&&
        (taxispos.m_uiRuleListIndex == v_taxis_pos_tmp[i].m_uiRuleListIndex)&&
        (taxispos.m_uiTopListIndex == v_taxis_pos_tmp[i].m_uiTopListIndex))
        {
            v_taxis_pos_tmp.erase(v_taxis_pos_tmp.begin()+i);
            /*如果用户的排序链表中只有1个节点，则删除该用户节点*/
            if(1 == uiVSize)
            {
                g_user_taxis_pos_list.erase(p_map_taxispos);
            }
            return 0;
        }
    }
    return -1;
}
/*************************************************************
*函数名称: InsertTaxisPosNode()
*输入参数: 略
*返回参数: 略
*函数功能: 在排序链表里面插入节点
************************************************************/
int CTopTaxisHandle::InsertTaxisPosNode(unsigned long long ullNewPopUin, Toptaxis_Pos taxispos)
{
    MAP_TAXIS_POS::iterator p_map_taxispos;
    MAP_TAXIS_POS::iterator p_map_taxisposend;

    V_Toptaxis_Pos v_toptaxis_pos;
    p_map_taxispos = g_user_taxis_pos_list.begin();
    p_map_taxisposend = g_user_taxis_pos_list.end();
    /*与系统相关的数据不用写入此链表里面*/
    if(E_REAL_DATA != g_condition_list[taxispos.m_uiBidIndex].m_vConditionList[taxispos.m_uiConditionListIndex].m_usTaxisTime)
    {
        DEBUG_LOG("InsertUserTaxisPosList[%llu]Bid[%u]Condition[%u]Result[None]", 
            ullNewPopUin, taxispos.m_uiBidIndex, taxispos.m_uiConditionListIndex);
        return 0;
    }
    //V_Toptaxis_Pos v_taxis_pos_tmp;
    for( ; p_map_taxispos != p_map_taxisposend; p_map_taxispos++)
    {
        if(ullNewPopUin == p_map_taxispos->first)
        {
            break;
        }
    }
    /*不存在，则做为新节点插入*/
    if(p_map_taxispos == p_map_taxisposend)
    {
        v_toptaxis_pos.push_back(taxispos);
        g_user_taxis_pos_list.insert(make_pair(ullNewPopUin, v_toptaxis_pos));
        return 0;
    }
    V_Toptaxis_Pos &v_taxis_pos_tmp = p_map_taxispos->second;
    v_taxis_pos_tmp.push_back(taxispos);
    return 0;
}
/*************************************************************
*函数名称: compare_msg_cond()
*输入参数: 略
*返回参数: 略
*函数功能: 字符串列表里面的所有字符串是否完全匹配
************************************************************/
int compare_msg_cond(vector <string>& r_vSrcString, vector <Field_Info>& r_vDesString)
{
    unsigned int ui_size= 0, i = 0;
    ui_size = r_vSrcString.size();
    if(ui_size != r_vDesString.size())
    {
        return -1;
    }
    for(i = 0; i < ui_size; i++)
    {
        if(r_vSrcString[i] == r_vDesString[i].m_sField)
        {
            continue;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}
/*************************************************************
*函数名称: assign_vector_list()
*输入参数: 略
*返回参数: 略
*函数功能: vector数据赋值
************************************************************/
int assign_vector_list(vector <unsigned int> &r_v_dsrlist, vector <unsigned int> &r_v_orglist)
{
    unsigned int ui_size = 0, i = 0;
    r_v_dsrlist.clear();
    ui_size = r_v_orglist.size();
    for(i = 0; i < ui_size; i++)
    {
        r_v_dsrlist.push_back(r_v_orglist[i]);
    }
    return 0;
}

/*************************************************************
*函数名称: write_water_log()
*输入参数: 略
*返回参数: 略
*函数功能: 写流水日志
************************************************************/
int CTopTaxisHandle::WriteTopShm(Top_Info& r_top_info)
{
    unsigned int i = 0, ui_keyvaluesize = 0;

    TopTaxisShmData* p_stTaxisShmData = NULL;

    ui_keyvaluesize = r_top_info.m_vKeyValueList.size();

    if((NULL == r_top_info.m_pDataAddr)||((void*)-1 == r_top_info.m_pDataAddr))
    {
        ERROR_LOG("shm[NULL]");
        return -1;
    }
    p_stTaxisShmData = (TopTaxisShmData*)r_top_info.m_pDataAddr;


    /*AB文件方式，为了获取到最后完整的数据*/
    if(('A' != p_stTaxisShmData->m_ucABFlag)&&('B' != p_stTaxisShmData->m_ucABFlag))
    {
        ERROR_LOG("shmflag[%c]", p_stTaxisShmData->m_ucABFlag);
        return -1;
    }
    g_cCommSemp.Lock();
    /*最后完整的是A块数据，则写B块区域*/
    if('A' == p_stTaxisShmData->m_ucABFlag)
    {
        /*排序列表*/
        for(i = 0; i < ui_keyvaluesize; i++)
        {
            p_stTaxisShmData->m_tBData[i].m_ullKey =  r_top_info.m_vKeyValueList[i].m_ullKey;
            p_stTaxisShmData->m_tBData[i].m_iValue = r_top_info.m_vKeyValueList[i].m_iValue;
            p_stTaxisShmData->m_tBData[i].m_iChangeValue = r_top_info.m_vKeyValueList[i].m_iChangeValue;
        }
        p_stTaxisShmData->m_uiBNum = ui_keyvaluesize;
        p_stTaxisShmData->m_ucABFlag = 'B';
    }
    else/*最后完整的是B块数据,则写A块区域*/
    {
        /*排序列表*/
        for(i = 0; i < ui_keyvaluesize; i++)
        {
            p_stTaxisShmData->m_tAData[i].m_ullKey =  r_top_info.m_vKeyValueList[i].m_ullKey;
            p_stTaxisShmData->m_tAData[i].m_iValue = r_top_info.m_vKeyValueList[i].m_iValue;
            p_stTaxisShmData->m_tAData[i].m_iChangeValue = r_top_info.m_vKeyValueList[i].m_iChangeValue;
        }
        p_stTaxisShmData->m_uiANum = ui_keyvaluesize;
        p_stTaxisShmData->m_ucABFlag = 'A';
    }
    g_cCommSemp.Unlock();
    return 0;
}

/*************************************************************
*函数名称: write_water_log()
*输入参数: 略
*返回参数: 略
*函数功能: 写流水日志
************************************************************/
int CTopTaxisHandle::WriteWaterLog(Sync_Toptaxis *p_sync_toptaxis, Top_Info& r_top_info, int i_cold_flag)
{
    char sz_record[TAXIS_MAX_RECORD_LEN] = {0};
    int i_retlen = 0;
    char *p_tmp = NULL;
    unsigned int ui_water_len = 0;
    unsigned int i = 0, ui_condsize = 0, ui_keyvaluesize = 0;

    if(NULL == p_sync_toptaxis)
    {
        ERROR_LOG("p_sync_toptaxis[NULL]");
        return -1;
    }

    ui_condsize = r_top_info.m_vUiConditionValue.size();
    ui_keyvaluesize = r_top_info.m_vKeyValueList.size();

    if(p_sync_toptaxis->m_uiConditionValueNum != ui_condsize)
    {
        ERROR_LOG("condsize[%u]ConditionValueNum[%u]", ui_condsize, p_sync_toptaxis->m_uiConditionValueNum);
        return -1;
    }
    if(p_sync_toptaxis->m_uiTopListNum > ui_keyvaluesize)
    {
        ERROR_LOG("keyvaluesize[%u]TopListNum[%u]", ui_keyvaluesize, p_sync_toptaxis->m_uiTopListNum);
        return -1;
    }

    //ui_keyvaluesize = p_sync_toptaxis->m_uiTopListNum;

    //bzero(g_sz_waterlogdata, TAXIS_MAX_WATER_LOG);

    i_retlen = snprintf(g_sz_waterlogdata, TAXIS_MAX_HEAD_WATER_LOG, "%u|%u|%u|%u|%u|%u", 
    p_sync_toptaxis->m_taxispos.m_uiBidIndex,
    p_sync_toptaxis->m_taxispos.m_uiConditionListIndex,
    p_sync_toptaxis->m_taxispos.m_uiRuleListIndex,
    p_sync_toptaxis->m_taxispos.m_uiTopListIndex,
    p_sync_toptaxis->m_uiConditionValueNum,
    p_sync_toptaxis->m_uiTopListNum);
    if(i_retlen < 0)
    {
        ERROR_LOG("i_retlen[%d]file[waterlog]", i_retlen);
        return -1;
    }
    /*条件值列表*/
    for(i = 0; i < ui_condsize; i++)
    {
        snprintf(sz_record, TAXIS_MAX_RECORD_LEN, "|%u",  r_top_info.m_vUiConditionValue[i]);
        p_tmp = strncat(g_sz_waterlogdata, sz_record, TAXIS_MAX_RECORD_LEN);
        if(NULL == p_tmp)
        {
            ERROR_LOG("strncat[NULL]file[waterlog]");
            return -1;
        }
    }
    /*排序列表*/
    for(i = 0; i < p_sync_toptaxis->m_uiTopListNum; i++)
    {
        snprintf(sz_record, TAXIS_MAX_RECORD_LEN, "|%llu_%d_%d",  r_top_info.m_vKeyValueList[i].m_ullKey, 
        r_top_info.m_vKeyValueList[i].m_iValue, r_top_info.m_vKeyValueList[i].m_iChangeValue);
        p_tmp = strncat(g_sz_waterlogdata, sz_record, TAXIS_MAX_RECORD_LEN);
        if(NULL == p_tmp)
        {
            ERROR_LOG("strncat[NULL]file[waterlog]");
            return -1;
        }
    }
    strncat(g_sz_waterlogdata, "\n", TAXIS_MAX_RECORD_LEN);
    ui_water_len = strlen(g_sz_waterlogdata);
    /*是否需要落共享内存*/
    if(E_TAXIS_COLD_WATERLOG != i_cold_flag)
    {
        /*当前是否在处理消息标志, 便于每个消息只记录一次流水*/
        if(1 == g_uiHandMsgFlag)
        {
            string strReqMsg = CDataProcess::Instance()->GetReqMsg();
            CTsWaterLog::Instance()->WriteWaterLog(strReqMsg.c_str(), (unsigned int)strReqMsg.size());
            g_uiHandMsgFlag = 0;
        }
        WriteTopShm(r_top_info);
        g_cNotifySync.NotifySyncMsg(r_top_info.m_uiShmId);
    }
    else
    {
        g_toptaxis_cold_waterlog.WriteLog(g_sz_waterlogdata, ui_water_len);
    }
    return 0;
}
/*************************************************************
*函数名称: create_toplist_file(void)
*输入参数: 略
*返回参数: 略
*函数功能: 创建记录top排序的文件，用于异常情况下的数据恢复
************************************************************/
int CTopTaxisHandle::CreateToplistFile(Top_Info& r_top_info, Toptaxis_Pos t_toptaxis_pos)
{
    char str_topfilename[MAX_FILE_PATH_LEN] = {0};
    char str_topfilename_base[MAX_FILE_PATH_LEN] = {0};
    char sz_record[TAXIS_MAX_UNSIGN_INT] = {0};
    unsigned int ui_size = 0, i = 0;
    unsigned int ui_rule_id = 0; 
    int shm_id = 0;
    FILE* p_file_tmp = NULL;
    TopTaxisShmData* p_stTaxisShmData = NULL;
    int i_ret = 0;
    char str_warnmsg[MAX_WARN_LEN] = {0};

    unsigned int uiBidSize = 0, uiCondSize = 0, uiRuleSize = 0;

    uiBidSize = g_condition_list.size();
    if(t_toptaxis_pos.m_uiBidIndex >= uiBidSize)
    {
        ERROR_LOG("OverLoad[!!!!!]BidSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]", 
            uiBidSize, 
            t_toptaxis_pos.m_uiBidIndex, t_toptaxis_pos.m_uiConditionListIndex, 
            t_toptaxis_pos.m_uiRuleListIndex, t_toptaxis_pos.m_uiTopListIndex);
        return 0;
    }
    Bid_Condition& rstBidTmp = g_condition_list[t_toptaxis_pos.m_uiBidIndex];
    uiCondSize = rstBidTmp.m_vConditionList.size();
    if(t_toptaxis_pos.m_uiConditionListIndex >= uiCondSize)
    {
        ERROR_LOG("OverLoad[!!!!!]CondSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]",
        uiCondSize,
        t_toptaxis_pos.m_uiBidIndex, t_toptaxis_pos.m_uiConditionListIndex, 
        t_toptaxis_pos.m_uiRuleListIndex, t_toptaxis_pos.m_uiTopListIndex);
        return 0;
    }
    Condition_Info& rstCondTmp = rstBidTmp.m_vConditionList[t_toptaxis_pos.m_uiConditionListIndex];
    uiRuleSize = rstCondTmp.m_vRuleList.size();
    if(t_toptaxis_pos.m_uiRuleListIndex >= uiRuleSize)
    {
        ERROR_LOG("OverLoad[!!!!!]RuleSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]TopIndex[%u]", 
        uiRuleSize, 
        t_toptaxis_pos.m_uiBidIndex, t_toptaxis_pos.m_uiConditionListIndex, 
        t_toptaxis_pos.m_uiRuleListIndex, t_toptaxis_pos.m_uiTopListIndex);
        return 0;
    }
    Rule_Info& rstRuleTmp = rstCondTmp.m_vRuleList[t_toptaxis_pos.m_uiRuleListIndex];
    
    ui_rule_id = rstRuleTmp.m_uiRuleId;
    snprintf(str_topfilename, MAX_FILE_PATH_LEN,  "%s/%s%u", TAXIS_LAST_DATA_DIR, TAXIS_LAST_DATA_PREFIX, ui_rule_id);
    ui_size = r_top_info.m_vUiConditionValue.size();
    snprintf(sz_record, TAXIS_MAX_UNSIGN_INT, "_%u", ui_size);
    strncat(str_topfilename, sz_record, TAXIS_MAX_UNSIGN_INT);
    for(i = 0; i < ui_size; i++)
    {
        snprintf(sz_record, TAXIS_MAX_UNSIGN_INT, "_%u", r_top_info.m_vUiConditionValue[i]);
        strncat(str_topfilename, sz_record, TAXIS_MAX_UNSIGN_INT);
    }

    r_top_info.m_lasttopfd = fopen(str_topfilename, "w+"); 
    if(NULL == r_top_info.m_lasttopfd)
    {
        ERROR_LOG("createposfile[%s]error[%d:%s]\n", str_topfilename, errno, strerror(errno));
        return ERROR_OTHER_FAILED;
    }

    strncat(str_topfilename, "\n", 1);
    g_ui_shm_max++;
    NOTI_LOG("g_ui_shm_max[%u]\n", g_ui_shm_max);
    r_top_info.m_uiShmId = g_ui_shm_max;

    shm_id = shmget(r_top_info.m_uiShmId, TAXIS_SHM_SIZE, 0640|IPC_CREAT|IPC_EXCL);
    if( -1 == shm_id)
    {
        WARN_LOG("reason[create shm failed]shmid[%u]keyno[%u]bid[%u]cond[%u]rule[%u]top[%u]error[%d]errmsg[%s]",
            r_top_info.m_uiShmId, TAXIS_SHM_SIZE,
            t_toptaxis_pos.m_uiBidIndex,
            t_toptaxis_pos.m_uiConditionListIndex,
            t_toptaxis_pos.m_uiRuleListIndex,
            t_toptaxis_pos.m_uiTopListIndex,
            errno,strerror(errno));
        snprintf(str_warnmsg, MAX_WARN_LEN, "create shm fail, id(%u)[%s]", r_top_info.m_uiShmId, strerror(errno));
        CWarnConf::Instance()->SendWarn(str_warnmsg);
        r_top_info.m_pDataAddr = NULL;
        fclose(r_top_info.m_lasttopfd);
        r_top_info.m_lasttopfd = NULL;
        return -1;
    }
    NOTI_LOG("create  data  cache succ !!!!!![%d] file[%s]", shm_id, str_topfilename);

    /*attach 共享内存*/
    r_top_info.m_pDataAddr  = (void*  )shmat(shm_id, NULL, 0);
    if((void*)-1 == r_top_info.m_pDataAddr)
    {
        WARN_LOG("reason[attach shm failed]keyno[%u]bid[%u]cond[%u]rule[%u]top[%u]error[%d]errmsg[%s]",r_top_info.m_uiShmId,
        t_toptaxis_pos.m_uiBidIndex,
        t_toptaxis_pos.m_uiConditionListIndex,
        t_toptaxis_pos.m_uiRuleListIndex,
        t_toptaxis_pos.m_uiTopListIndex,
        errno,strerror(errno));
        snprintf(str_warnmsg, MAX_WARN_LEN, "attach shm fail, id(%u)[%s]", r_top_info.m_uiShmId, strerror(errno));
        CWarnConf::Instance()->SendWarn(str_warnmsg);
        fclose(r_top_info.m_lasttopfd);
        r_top_info.m_lasttopfd = NULL;
        return -1;
    }

    p_stTaxisShmData = (TopTaxisShmData*)r_top_info.m_pDataAddr;
    /*共享内存的key写入文件*/
    bzero(g_sz_waterlogdata, TAXIS_MAX_WATER_COLD_LOG);

    snprintf(g_sz_waterlogdata, TAXIS_MAX_UNSIGN_INT, "%u\n", r_top_info.m_uiShmId);	

    fwrite(g_sz_waterlogdata, 1, TAXIS_MAX_UNSIGN_INT, r_top_info.m_lasttopfd);
    fclose(r_top_info.m_lasttopfd);
    r_top_info.m_lasttopfd = NULL;

    p_file_tmp = rstRuleTmp.m_posfd;
    if(NULL == p_file_tmp)
    {
        snprintf(str_topfilename_base, MAX_FILE_PATH_LEN,  "%s/%s%u", TAXIS_LAST_DATA_DIR, TAXIS_LAST_DATA_PREFIX, ui_rule_id);
        p_file_tmp = fopen(str_topfilename_base, "r+"); 
        if(NULL == p_file_tmp)
        {
            ERROR_LOG("OpenPosFile[fail]Filename[%s]error[%d:%s]", str_topfilename_base, errno, strerror(errno));
            return ERROR_OTHER_FAILED;
        }
    }
    i_ret = fseek( p_file_tmp, 0L, SEEK_END);
    if(-1 == i_ret)
    {
        ERROR_LOG("SeekEndFail[%s]BidIndex[%u]ConditionListIndex[%u]RuleListIndex[%u]top[%u]ruleid[%u]code[noposfile]", 
            str_topfilename,
            t_toptaxis_pos.m_uiBidIndex,
            t_toptaxis_pos.m_uiConditionListIndex,
            t_toptaxis_pos.m_uiRuleListIndex,
            t_toptaxis_pos.m_uiTopListIndex,
            ui_rule_id);
        fclose(p_file_tmp); 
        return -1;
    }

    fwrite(str_topfilename, 1, strlen(str_topfilename), p_file_tmp);
    fclose(p_file_tmp); 
    g_condition_list[t_toptaxis_pos.m_uiBidIndex].m_vConditionList[t_toptaxis_pos.m_uiConditionListIndex].m_vRuleList[t_toptaxis_pos.m_uiRuleListIndex].m_posfd = NULL;

    ui_size = r_top_info.m_vUiConditionValue.size();
    p_stTaxisShmData->m_usCondNum = ui_size;
    for(i = 0; i < ui_size; i++)
    {
        if(i >= TAXIS_MAX_COND_NUM)
        {
            EMERG_LOG("CondNum[%u]OverMaxNum[%u]", ui_size, TAXIS_MAX_COND_NUM);
            break;
        }
        p_stTaxisShmData->m_auiCond[i] = r_top_info.m_vUiConditionValue[i];
    }

    p_stTaxisShmData->m_stTaxisPos.m_uiBidIndex = t_toptaxis_pos.m_uiBidIndex;
    p_stTaxisShmData->m_stTaxisPos.m_uiConditionListIndex = t_toptaxis_pos.m_uiConditionListIndex;
    p_stTaxisShmData->m_stTaxisPos.m_uiRuleListIndex = t_toptaxis_pos.m_uiRuleListIndex;
    p_stTaxisShmData->m_stTaxisPos.m_uiTopListIndex = t_toptaxis_pos.m_uiTopListIndex;

    p_stTaxisShmData->m_ucABFlag = 'B';
    p_stTaxisShmData->m_uiANum = 0;
    p_stTaxisShmData->m_uiBNum = 0;
    return 0;
}
/*************************************************************
*函数名称: handle_msg()
*输入参数: 略
*返回参数: 略
*函数功能: 处理从DC_Agent过来的同步数据
************************************************************/
int CTopTaxisHandle::HandleMsg(Toptaxis_Msg& toptaxis_msg)
{
    unsigned int ui_ConditionIndex = 0, ui_BidIndex = 0, k = 0, ui_TopListIndex = 0, ui_RuleIndex = 0, q = 0, i =0;
    unsigned int uiMsgConditionNum = 0, uiConfConditionNum = 0,uiTmp = 0;
    unsigned int uiTmp1 = 0, uiTmp3 = 0, uiTmp4 = 0;
    unsigned long long ullPopUinTmp = 0;

    int iRet = 0;
    Sync_Toptaxis t_sync_toptaxis;
    vector <unsigned int> v_value_list;
    Sort_Pos_Info t_posinfo; 

    int iDataTemp = 0;
    unsigned int ui_size = 0;
    DEBUG_LOG("Begin handle_msg Number[%llu]Bid[%u]CondChange[%u]OptType[%u]", toptaxis_msg.m_ullNumber, 
        toptaxis_msg.m_uiBid, toptaxis_msg.m_usCondChange, toptaxis_msg.m_usOptType);
    ui_size = toptaxis_msg.m_vConditionInfo.size();
    for(i = 0; i < ui_size; i++)
    {
        DEBUG_LOG("Condition%u[%s]value[%u]", i, toptaxis_msg.m_vConditionInfo[i].m_sField.c_str(), 
            toptaxis_msg.m_vConditionInfo[i].m_uiValue);
    }
    ui_size = toptaxis_msg.m_vFieldInfo.size();
    for(i = 0; i < ui_size; i++)
    {
        DEBUG_LOG("Field%u[%s]value[%u]", i, toptaxis_msg.m_vFieldInfo[i].m_sField.c_str(), 
            toptaxis_msg.m_vFieldInfo[i].m_uiValue);
    }    
    
    for(ui_BidIndex = 0; ui_BidIndex < g_uiBidNum; ui_BidIndex++)
    {
        if(g_condition_list[ui_BidIndex].m_uiBid == toptaxis_msg.m_uiBid)
        {
            break;
        }
    }
    /*业务ID不存在，返回失败*/
    if(g_uiBidNum == ui_BidIndex)
    {
        ERROR_LOG("BidNum[%u]Bid[%u]Code[NotExist]", g_uiBidNum, toptaxis_msg.m_uiBid);
        return -1;
    }

    /*用户数据的删除则直接删除*/
    if((E_TAXIS_MSG_OPT_DEL == toptaxis_msg.m_usOptType)&&(E_TAXIS_DATA_TYPE_USER == toptaxis_msg.m_usDataType))
    {
        DelUinAllTaxis(toptaxis_msg.m_ullNumber, ui_BidIndex);
        DEBUG_LOG("DelUinAllTaxis[OK]");
        return 0;
    }
    
    /*白名单号码的消息不处理直接返回*/
    if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vWhiteList, toptaxis_msg.m_ullNumber))
    {
        INFO_LOG("WhiteList[%llu]Pos[BidLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
        return 0;
    }

    
    uiMsgConditionNum = toptaxis_msg.m_vConditionInfo.size();
    uiConfConditionNum = g_condition_list[ui_BidIndex].m_vConditionList.size();
    /*遍历规则文件中的条件，找到消息中与之匹配的条件*/
    /*规则文件中的条件列表*/
    for(ui_ConditionIndex = 0; ui_ConditionIndex < uiConfConditionNum; ui_ConditionIndex++)
    {
        uiTmp = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vCondition.size();
        /*白名单号码的消息不处理直接返回*/
        if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vWhiteList, toptaxis_msg.m_ullNumber))
        {
            INFO_LOG("WhiteList[%llu]Pos[CondLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
            continue;
        }
        v_value_list.clear();
        /*如果有条件则要求条件一定匹配*/
        if(0 != uiTmp)
        {
            if(0 == compare_msg_cond(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vCondition, 
                toptaxis_msg.m_vConditionInfo))
            {
                //v_value_list.clear();
                /*将消息中条件属性的值保存下来*/
                DEBUG_LOG("BidIndex[%u]ConditionList[%u]Code[ConditionOK]", ui_BidIndex, ui_ConditionIndex);
                for(k = 0; k < uiMsgConditionNum; k++)
                {
                    v_value_list.push_back(toptaxis_msg.m_vConditionInfo[k].m_uiValue);
                }
            }
            else
            {
                continue;
            }
        }
        /*规则文件条件的每一个子条件在消息里面都存在，则进行排序
        或者是条件中无任何条件，则直接匹配规则进行排序*/
        /*条件值发生变化，比如城市发生变化,则删除老数据*/
        if(0 != toptaxis_msg.m_usCondChange)
        {
            /*系统类的数据不允许条件值发生变化，比如股票就不能发生城市变化，固定在一个城市*/
            if(E_TAXIS_DATA_TYPE_SYSTEM == toptaxis_msg.m_usDataType)
            {
                string strReqMsg = CDataProcess::Instance()->GetReqMsg();
                EMERG_LOG("ERRORMSG[discard]Uin[%llu]Msg[%s]", toptaxis_msg.m_ullNumber, strReqMsg.c_str());
                return -1;
            }
            Toptaxis_Pos t_toptaxis_pos_tmp;
            t_toptaxis_pos_tmp.m_uiBidIndex = ui_BidIndex;
            t_toptaxis_pos_tmp.m_uiConditionListIndex = ui_ConditionIndex;
            DelTaxisPosRule(toptaxis_msg.m_ullNumber, t_toptaxis_pos_tmp);
        }
        uiTmp3 = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList.size();
        uiTmp4 = toptaxis_msg.m_vFieldInfo.size();
        /*遍历规则文件里面某条件的所有规则属性*/
        for(ui_RuleIndex = 0; ui_RuleIndex < uiTmp3; ui_RuleIndex++)
        {
            /*如果规则不生效，则也不进行排序*/
            if(E_RULE_DISABLE == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_cEnableFlag)
            {
                INFO_LOG("BidIndex[%u]ConditionIndex[%u]RuleIndex[%u]Result[RULEDISABLE]", ui_BidIndex, ui_ConditionIndex, ui_RuleIndex);
                continue;
            }
            /*白名单号码的消息不处理直接返回*/
            if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vWhiteList, 
                toptaxis_msg.m_ullNumber))
            {
                INFO_LOG("WhiteList[%llu]Pos[RuleLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
                continue;
            }
            for(q = 0; q < uiTmp4; q++)
            {
                /*规则文件的规则在消息里面(属性)匹配一致，则进行排序*/
                if(toptaxis_msg.m_vFieldInfo[q].m_sField == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_sFieldId)
                {
                    uiTmp1 = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList.size();
                    for(ui_TopListIndex = 0; ui_TopListIndex < uiTmp1; ui_TopListIndex++)
                    {
                        if(0 == VectorUndataCompare(v_value_list, g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList[ui_TopListIndex].m_vUiConditionValue))
                        {
                            DEBUG_LOG("BidIndex[%u]ConditionList[%u]RuleList[%u]TopList[%u]Code[ConditionValueOK]", 
                            ui_BidIndex, ui_ConditionIndex, ui_RuleIndex, ui_TopListIndex);

                            iDataTemp = (int)toptaxis_msg.m_vFieldInfo[q].m_uiValue;
                            /*需要保存当前值的条件规则*/
                            if(E_REAL_DATA != g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usTaxisTime)
                            {
                                UpdateCurData(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_mapHostoryList,
                                    toptaxis_msg.m_vFieldInfo[q].m_uiValue,
                                    toptaxis_msg.m_ullNumber,
                                    &iDataTemp,
                                    g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_historyfd,
                                    g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usTaxisTime);
                            }
                            t_sync_toptaxis.m_taxispos.m_uiBidIndex = ui_BidIndex;
                            t_sync_toptaxis.m_taxispos.m_uiConditionListIndex = ui_ConditionIndex;
                            t_sync_toptaxis.m_taxispos.m_uiRuleListIndex = ui_RuleIndex;
                            t_sync_toptaxis.m_taxispos.m_uiTopListIndex = ui_TopListIndex;

                            /*符合条件 规则的属性则进行排序*/
                            iRet = VectorUndataSort(t_sync_toptaxis.m_taxispos,
                                toptaxis_msg.m_vFieldInfo[q].m_uiValue,
                                iDataTemp,
                                toptaxis_msg.m_ullNumber,
                                g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_uiTopNum, 
                                g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_usTaxisType,
                                ullPopUinTmp, t_posinfo);
                            /*原排序的数据有调整，则落流水进行同步*/
                            if(0 == iRet)
                            {
                                DEBUG_LOG("BidIndex[%u]ConditionList[%u]RuleList[%u]TopList[%u]Code[SortData]", 
                                    ui_BidIndex, ui_ConditionIndex, ui_RuleIndex, ui_TopListIndex);
                                /*排行榜中有用户下榜，则删除用户排序链数据. 系统相关的数据则不用删除*/
                                if((0 != ullPopUinTmp)&&
                                    (E_REAL_DATA == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usTaxisTime))
                                {
                                    DelTaxisPosNode(ullPopUinTmp, t_sync_toptaxis.m_taxispos);
                                }

                                t_sync_toptaxis.m_uiConditionValueNum = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList[ui_TopListIndex].m_vUiConditionValue.size();
                                t_sync_toptaxis.m_uiTopListNum = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList[ui_TopListIndex].m_vKeyValueList.size();
                                if(t_sync_toptaxis.m_uiTopListNum > g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_uiTopDisplayNum)
                                {
                                    t_sync_toptaxis.m_uiTopListNum = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_uiTopDisplayNum;	
                                }
                                /*写流水日志*/
                                WriteWaterLog(&t_sync_toptaxis,
                                    g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList[ui_TopListIndex],
                                    E_TAXIS_REAL_WATERLOG);
                            }
                            break;
                        }
                    }
                    /*如果一定条件规则内还不存在排序，则新创立新的排序*/
                    if(ui_TopListIndex == uiTmp1)
                    {
                        Top_Info tmp_topinfo;
                        Key_Value tmp_keyvalue;
                        tmp_keyvalue.m_iValue = toptaxis_msg.m_vFieldInfo[q].m_uiValue;
                        /*需要保存当前值的条件规则*/
                        if(E_REAL_DATA != g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usTaxisTime)
                        {
                            UpdateCurData(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_mapHostoryList,
                            toptaxis_msg.m_vFieldInfo[q].m_uiValue,
                            toptaxis_msg.m_ullNumber,
                            &iDataTemp,
                            g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_historyfd,
                            g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usTaxisTime);
                            tmp_keyvalue.m_iChangeValue  = iDataTemp; 
                        }
                        else
                        {
                            tmp_keyvalue.m_iChangeValue  = 0;
                        }
                        assign_vector_list(tmp_topinfo.m_vUiConditionValue, v_value_list);
                        tmp_keyvalue.m_ullKey = toptaxis_msg.m_ullNumber;

                        tmp_topinfo.m_vKeyValueList.push_back(tmp_keyvalue);

                        t_sync_toptaxis.m_taxispos.m_uiBidIndex = ui_BidIndex;
                        t_sync_toptaxis.m_taxispos.m_uiConditionListIndex = ui_ConditionIndex;
                        t_sync_toptaxis.m_taxispos.m_uiRuleListIndex = ui_RuleIndex;
                        t_sync_toptaxis.m_taxispos.m_uiTopListIndex = ui_TopListIndex;

                        CreateToplistFile(tmp_topinfo, t_sync_toptaxis.m_taxispos);
                        InsertTaxisPosNode(toptaxis_msg.m_ullNumber, t_sync_toptaxis.m_taxispos);

                        t_sync_toptaxis.m_uiConditionValueNum = v_value_list.size();
                        t_sync_toptaxis.m_uiTopListNum = 1;
                        /*写流水日志*/
                        WriteWaterLog(&t_sync_toptaxis, tmp_topinfo, E_TAXIS_REAL_WATERLOG);
                        g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList.push_back(tmp_topinfo);		
                    }
                }
            }
        }
    }
    return 0;
}
/*******************************************************************************
* Description: 在系统配置规则里面增加一些新规则
*       Input: 
*              
*      Output: 
*      Return: 0 成功 其它值失败
*      Others: 无
*******************************************************************************/
int AddCondRule(unsigned int uiBidIndex, Condition_Info& stCondInfo)
{
    unsigned short usCondId = 0;
    unsigned int uiCondIndex = 0, uiConfConditionNum = 0, uiConfRuleNum = 0, uiRuleIndex = 0, uiRuleId = 0;
    unsigned int uiWhiteSize = 0, uiWhiteIndex = 0;
    Toptaxis_Pos tRuleTopTaxisPos;
    usCondId = stCondInfo.m_usCondId;
    if(uiBidIndex > g_uiBidNum)
    {
        ERROR_LOG("BidIndex[OverMax]BidIndex[%u]BidNum[%u]", uiBidIndex, g_uiBidNum);
        return -1;
    }

    uiConfConditionNum = g_condition_list[uiBidIndex].m_vConditionList.size();
    tRuleTopTaxisPos.m_uiBidIndex = uiBidIndex;
    INFO_LOG("RuleBeginReload[CondLevel]BidIndex[%u]", uiBidIndex);
    /*遍历规则文件中的条件，找到消息中与之匹配的条件*/
    /*规则文件中的条件列表*/
    for(uiCondIndex = 0; uiCondIndex < uiConfConditionNum; uiCondIndex++)
    {
        Condition_Info &rCondInfo = g_condition_list[uiBidIndex].m_vConditionList[uiCondIndex];  
        if(usCondId == rCondInfo.m_usCondId)
        {
            INFO_LOG("WhiteListBeginReload[CondLevel]CondId[%u]", usCondId);
            /*白名单需要重新导入*/
            rCondInfo.m_vWhiteList.clear();
            uiWhiteSize = stCondInfo.m_vWhiteList.size();
            for(uiWhiteIndex = 0; uiWhiteIndex < uiWhiteSize; uiWhiteIndex++)
            {
                INFO_LOG("WhiteList[%llu]", stCondInfo.m_vWhiteList[uiWhiteIndex]);
                rCondInfo.m_vWhiteList.push_back(stCondInfo.m_vWhiteList[uiWhiteIndex]);
            }
            INFO_LOG("WhiteListEndReload[CondLevel]CondId[%u]", usCondId);
            uiConfRuleNum = stCondInfo.m_vRuleList.size();
            for(uiRuleIndex = 0; uiRuleIndex < uiConfRuleNum; uiRuleIndex++)      
            {
                uiRuleId = stCondInfo.m_vRuleList[uiRuleIndex].m_uiRuleId;
                /*该规则与已有的规则ID重复，不需要重复添加，只需要更新白名单即可(前面的流程已经更新过了)*/
                if(0 == uiRuleId)
                {
                    continue;
                }
                tRuleTopTaxisPos.m_uiConditionListIndex = uiCondIndex;
                tRuleTopTaxisPos.m_uiRuleListIndex = rCondInfo.m_vRuleList.size();
                
                INFO_LOG("RuleReloadOldCondInsert[%u]Bid[%u]Cond[%u]Rule[%u]", uiRuleId, tRuleTopTaxisPos.m_uiBidIndex, 
                    tRuleTopTaxisPos.m_uiConditionListIndex, tRuleTopTaxisPos.m_uiRuleListIndex);
                /*系统所有的规则链表*/
                gMapRuleList.insert(make_pair(uiRuleId, tRuleTopTaxisPos));                
                rCondInfo.m_vRuleList.push_back(stCondInfo.m_vRuleList[uiRuleIndex]);
            }
            INFO_LOG("RuleOldCondEndReload[RuleLevel]BidIndex[%u]", uiBidIndex);
            return 0;
        }
    }
    tRuleTopTaxisPos.m_uiConditionListIndex = g_condition_list[uiBidIndex].m_vConditionList.size();
    uiConfRuleNum = stCondInfo.m_vRuleList.size();
    /*更新规则链表*/
    for(uiRuleIndex = 0; uiRuleIndex < uiConfRuleNum; uiRuleIndex++)      
    {
        tRuleTopTaxisPos.m_uiRuleListIndex = uiRuleIndex;
        uiRuleId = stCondInfo.m_vRuleList[uiRuleIndex].m_uiRuleId;
        INFO_LOG("RuleReloadNewCondInsert[%u]Bid[%u]Cond[%u]Rule[%u]", uiRuleId, tRuleTopTaxisPos.m_uiBidIndex, 
            tRuleTopTaxisPos.m_uiConditionListIndex, tRuleTopTaxisPos.m_uiRuleListIndex);
        /*系统所有的规则链表*/
        gMapRuleList.insert(make_pair(uiRuleId, tRuleTopTaxisPos));                
    }
    g_condition_list[uiBidIndex].m_vConditionList.push_back(stCondInfo);
    INFO_LOG("RuleNewCondEndReload[RuleLevel]BidIndex[%u]", uiBidIndex);
    return 0;    
}
/*******************************************************************************
* Description: 更新当前的系统配置规则
*       Input: 
*              
*      Output: 
*      Return: 0 成功 其它值失败
*      Others: 无
*******************************************************************************/
int UpdateSysRuleConf(void)
{
    unsigned int uiConfConditionNum = 0, uiBidIndex = 0;

    unsigned int uiReloadCondIndex = 0, uiReloadConfCondNum = 0;
    unsigned int uiReloadBidIndex = 0, uiReloadBidNum = 0;
    unsigned int uiWhiteSize = 0, uiWhiteIndex = 0;
    unsigned char ucOKFlag = 0;
    uiReloadBidNum = gReloadCondList.size();
    INFO_LOG("CondBeginReload[BidLevel]");
    for(uiBidIndex = 0; uiBidIndex < g_uiBidNum; uiBidIndex++)
    {
        for(uiReloadBidIndex = 0; uiReloadBidIndex < uiReloadBidNum; uiReloadBidIndex++)
        {
            INFO_LOG("OldBid[%u]OldBidIndex[%u]OldBidTotal[%u]ReloadBid[%u]Index[%u]Total[%u]", 
                g_condition_list[uiBidIndex].m_uiBid, uiBidIndex, g_uiBidNum,
                gReloadCondList[uiReloadBidIndex].m_uiBid, uiReloadBidIndex, uiReloadBidNum);
            if(gReloadCondList[uiReloadBidIndex].m_uiBid == g_condition_list[uiBidIndex].m_uiBid)
            {
                /*白名单需要重新导入*/
                Bid_Condition &rBidCond = g_condition_list[uiBidIndex];
                rBidCond.m_vWhiteList.clear();
                uiWhiteSize = gReloadCondList[uiReloadBidIndex].m_vWhiteList.size();
                INFO_LOG("WhiteListBeginReload[BidLevel]Bid[%u]", g_condition_list[uiBidIndex].m_uiBid);
                for(uiWhiteIndex = 0; uiWhiteIndex < uiWhiteSize; uiWhiteIndex++)
                {
                    INFO_LOG("WhiteList[%llu]", gReloadCondList[uiReloadBidIndex].m_vWhiteList[uiWhiteIndex]);
                    rBidCond.m_vWhiteList.push_back(gReloadCondList[uiReloadBidIndex].m_vWhiteList[uiWhiteIndex]);
                }
                INFO_LOG("WhiteListEndReload[BidLevel]Bid[%u]", g_condition_list[uiBidIndex].m_uiBid);
                /*跳出二层循环*/
                ucOKFlag = 1;
                break;
            }
        }
        if(1 == ucOKFlag)
        {
            break;
        }
    }
    /*新业务ID，则添加为新业务*/
    if(g_uiBidNum == uiBidIndex)
    {
        for(uiReloadBidIndex = 0; uiReloadBidIndex < uiReloadBidNum; uiReloadBidIndex++)
        {
            INFO_LOG("NewBidReload[BidLevel]Bid[%u]", gReloadCondList[uiReloadBidIndex].m_uiBid); 
            g_condition_list.push_back(gReloadCondList[uiReloadBidIndex]);
        }
        g_uiBidNum += uiReloadBidNum;
        INFO_LOG("NewBidEndReload[BidLevel]");
        return 0;
    }
    Bid_Condition &rBidCond = g_condition_list[uiBidIndex];
    Bid_Condition &rReloadBidCond = gReloadCondList[uiReloadBidIndex];
    uiConfConditionNum = rBidCond.m_vConditionList.size();
    uiReloadConfCondNum = rReloadBidCond.m_vConditionList.size();
    for(uiReloadCondIndex = 0; uiReloadCondIndex < uiReloadConfCondNum; uiReloadCondIndex++) 
    {
        Condition_Info &rReloadCondInfo = rReloadBidCond.m_vConditionList[uiReloadCondIndex];  
        INFO_LOG("OldBidReload[BidLevel]CondId[%u]", rReloadCondInfo.m_usCondId);    
        AddCondRule(uiBidIndex, rReloadCondInfo);
    }
    INFO_LOG("OldBidEndReload[BidLevel]");
    return 0;
}
/*******************************************************************************
* Description: 扫描获取指定目录下面的所有xml文件
*       Input: strDir 指定的目录路径
*              
*      Output: rFileList 指定目录路径下所有xml文件的完整路径列表
*      Return: 0 成功 其它值失败
*      Others: 无
*******************************************************************************/
int GetDirXmlFile(const char* strDir, vector<string> &rFileList)
{
    if(NULL == strDir)
    {
        ERROR_LOG("[Dir is null]");
        return -1;
    }
    string strFilePath  = strDir;

    if(strFilePath[strFilePath.size() - 1] != '/')
    {
        strFilePath += "/";
    }

    char szRightName[RULE_FILE_NAME_LEN];

    string strTmpFilePath = strFilePath;

    struct stat stStatBuf;
    
    while(1)
    {
        snprintf(szRightName, sizeof(szRightName), "rule_conf_%d.xml", g_uiRuleFileNumPos);
         
        strFilePath += szRightName;
        if(access(strFilePath.c_str(), F_OK))
        {
            break;
        }
        
        if(stat(strFilePath.c_str(), &stStatBuf))
        {
            ERROR_LOG("file[%s]errno[%d] info[%s]", strFilePath.c_str(), errno, strerror(errno));
            return -2;
        }
        
        g_mFileLastChange[strFilePath.c_str()] = (unsigned)(stStatBuf.st_ctime);
        
        g_uiRuleFileNumPos++;

        rFileList.push_back(strFilePath);

        strFilePath = strTmpFilePath;
    }

    return 0;
}

/*******************************************************************************
* Description: 扫描获取指定目录下面的所有变化的xml文件
*       Input: strDir 指定的目录路径
*              
*      Output: rFileList 指定目录路径下所有发生变化的xml文件完整路径列表
*      Return: 0 成功 其它值失败
*      Others: 无
*******************************************************************************/
int ScanDirXmlFile(const char* strDir, vector<string> &rFileList)
{
    /*获取修改的配置文件*/
    struct stat stStatBuf;

    map<string, unsigned>::iterator mItBegin = g_mFileLastChange.begin();
    map<string, unsigned>::iterator mItEnd = g_mFileLastChange.end();
    
    for(map<string, unsigned>::iterator mItPos = mItBegin; mItPos != mItEnd; mItPos++)
    {
        if(stat(mItPos->first.c_str(), &stStatBuf))
        {
            ERROR_LOG("file[%s]errno[%d] info[%s]", mItPos->first.c_str(), errno, strerror(errno));
            return -2;
        }

        if(mItPos->second < (unsigned)(stStatBuf.st_ctime))
        {
            mItPos->second = (unsigned)(stStatBuf.st_ctime);

            rFileList.push_back(mItPos->first);
        }    
    }

    /*获取新加配置文件*/
    return GetDirXmlFile(strDir, rFileList);
}
/*******************************************************************************
* Description: 扫描规则配置文件的改动，并进行动态导入
*       Input: 
*              
*      Output: 
*      Return: 0 成功 其它值失败
*      Others: 无
*******************************************************************************/
int ScanRuleSysConf(void)
{
    unsigned int uiRuleFileNum = 0, uiRuleFileIndex = 0, uiConfBid = 0;
    int iCount = 0;
    vector <string> vRuleFileList;

    vRuleFileList.clear();

    ScanDirXmlFile(RULE_FILE_PATH, vRuleFileList);

    uiRuleFileNum = vRuleFileList.size();
    if(uiRuleFileNum > 0)
    {
        INFO_LOG("ScanRuleSysConfNum[%u]", uiRuleFileNum);
    }
    for(uiRuleFileIndex = 0; uiRuleFileIndex < uiRuleFileNum; uiRuleFileIndex++)
    {
        iCount = g_MapRuleConfBid.count(vRuleFileList[uiRuleFileIndex]);
        /*已存在的规则配置文件*/
        if(iCount > 0)
        {
            uiConfBid = g_MapRuleConfBid[vRuleFileList[uiRuleFileIndex]];        
        }
            
        if(g_pCRuleConf->ReloadConf(vRuleFileList[uiRuleFileIndex].c_str(), uiConfBid))
        {
            ERROR_LOG("ReloadRuleConf[Error]TaxisRuleConf[%s]Result[StopReload]", vRuleFileList[uiRuleFileIndex].c_str());
            return -1;
        }

        if(CDataProcess::Instance()->Init())
        {
            EMERG_LOG("CDataProcess Init error");
            return -1;
        } //-----todo
        /*新规则配置文件则需要新增规则文件与bid的映射关系*/
        if(iCount <= 0)
        {
            INFO_LOG("ReloadInsert[Conf-Bid]ConfFile[%s]Bid[%u]", vRuleFileList[uiRuleFileIndex].c_str(), uiConfBid);
            g_MapRuleConfBid.insert(make_pair(vRuleFileList[uiRuleFileIndex], uiConfBid));
        }
        
        INFO_LOG("BeginReloadRuleFile[%s]Index[%u]", vRuleFileList[uiRuleFileIndex].c_str(), uiRuleFileIndex);
        UpdateSysRuleConf();
        INFO_LOG("EndReloadRuleFile[%s]Index[%u]", vRuleFileList[uiRuleFileIndex].c_str(), uiRuleFileIndex);
    }
    return 0;
}

