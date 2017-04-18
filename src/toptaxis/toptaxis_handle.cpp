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
*      Description:   ��ϵͳ��������ݽ��д����γ�Top��������
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
extern vector <Bid_Condition> g_condition_list;/*ϵͳ�����������б�*/

extern vector <Bid_Condition> gReloadCondList;

extern map<unsigned int, set<string> > g_cond_list; /*ϵͳ�����������б�*/
extern map<unsigned int, set<string> > g_field_list; /*ϵͳ�����������б�*/
extern unsigned int g_uiBidNum; /*ϵͳ��ҵ����Ŀ*/
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

/*ȫ�ִ���������Ϣ�Ķ���*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

/*ϵͳ�����еĹ���*/
extern MAP_TAXIS_RULE_LIST gMapRuleList;

/*����������*/
extern CRuleConf* g_pCRuleConf;

map<string, unsigned int> g_mFileLastChange;

unsigned int g_uiRuleFileNumPos;

/*ȫ�ֹ��������ļ���Bid�Ĺ�ϵӳ���*/
extern MAP_TAXIS_RULECONF_BID g_MapRuleConfBid;

extern CTaxisServiceConf * g_pCServiceConf;

/*��ǰ�Ƿ��ڴ�����Ϣ��־, ����ÿ����Ϣֻ��¼һ����ˮ*/
extern unsigned int g_uiHandMsgFlag;

/*************************************************************
*��������: FindWhiteList()
*�������: ��
*���ز���: ��
*��������: �ж��Ƿ��ڰ�����֮��
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
*��������: vector_undata_sort()
*�������: ��
*���ز���: ��
*��������: ������������������ݽ�������
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
    /*��������б������и��û������ݣ���ժ������������*/
    for(i_first_pos = 0; i_first_pos < i_vector_len; i_first_pos++)
    {
        if(v_ListOrg[i_first_pos].m_ullKey == ullUin)
        {
            i_old_value = v_ListOrg[i_first_pos].m_iValue;
            i_old_change = v_ListOrg[i_first_pos].m_iChangeValue;
            /*�������ֵû�б仯���򲻽�������*/
            if(((i_old_value == iCurData)&&(E_REAL_DATA == us_type))
                ||((i_old_change == uiData)&&(E_REAL_DATA != us_type)))
            {
                if(E_REAL_DATA != us_type)
                {
                    /*��Ʊ�ĵ�ǰֵ�б仯��Ҳ��Ҫ����*/
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
            /*�����������д��ڣ����漴ʹ�ٲ���Ҳ�������û����������м�¼*/
            i_exist_toptaix = 1;
            i_vector_len--;
            break;
        }
    }
    r_posinfo.m_i_firstpos =  i_first_pos;
    DEBUG_LOG("Uin[%llu]Value[%d]Code[Push]", ullUin, uiData);
    /*�������У���Сֵ������ǰ��*/
    if(E_TOP_DOWN == usTaixType)
    {
        /*�������ͨ������ֻ�Ե�ǰֵ����*/
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
        else/*������ǵ���������Ըı�ֵ����*/
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
        /*��������ֵ����������*/
        if(i_last_pos == i_vector_len)
        {
            if(i_vector_len >= (int)uiSortNum)
            {
                /*�����ɾ������˵��������б仯����Ҫд��ˮ*/
                if(1 == i_exist_toptaix)
                {
                    r_posinfo.m_i_lastpos =  i_first_pos;
                    return 0;
                }
                /*���ֵ�����������*/
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
        /*�������ͨ������ֻ�Ե�ǰֵ����*/
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
        else/*������ǵ���������Ըı�ֵ����*/
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
        /*�������Сֵ����������*/
        if(i_last_pos == i_vector_len)
        {
            if(i_vector_len >= (int)uiSortNum)
            {
                /*�����ɾ������˵��������б仯����Ҫд��ˮ*/
                if(1 == i_exist_toptaix)
                {
                    r_posinfo.m_i_lastpos =  i_first_pos;
                    return 0;
                }
                /*��Сֵ�����������*/
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
    /*���»�ȡ����������Ľڵ���Ŀ*/
    i_vector_len = v_ListOrg.size();
    /*��������ڵ���������ɾ�����һ���ڵ�*/
    if((i_vector_len > (int)uiSortNum)&&(i_vector_len > 0))
    {
        ullPopUin = v_ListOrg[i_vector_len-1].m_ullKey;
        v_ListOrg.pop_back();
    }
    return 0;
}

/*************************************************************
*��������: vector_undata_compare()
*�������: ��
*���ز���: ��
*��������: �Ƚ�2��vector�������Ƿ���ȫ���
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
*��������: update_cur_data()
*�������: ��
*���ز���: ��
*��������: ������Ҫ������ʷ���ݵĵ�ǰֵ
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
    /*�½ӵ�����Ҫ���ó�ʼֵ�͵�ǰֵ*/
    t_history_temp.m_vuiInitData.push_back(uiValue);
    t_history_temp.m_uiCurData = uiValue;
    map_hostory.insert(make_pair(ullUin, t_history_temp));
    *iDiffData = 0;
    /*�½ڵ�����Ҫ����ʷ�ļ��в����¼*/
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
*��������: del_taxis_top_list_node()
*�������: ��
*���ز���: ��
*��������: ɾ��������������Ľڵ�
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

    /*����д��ˮ*/
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

            /*���¹����ڴ�ͬ��������*/
            WriteWaterLog(&stSyncTopTaxis,
                rstRuleTmp.m_vTopList[taxispos.m_uiTopListIndex],
                E_TAXIS_REAL_WATERLOG);
            return 0;
        }
    }
    return -1;
}
/*************************************************************
*��������: DelUinAllTaxis()
*�������: ��
*���ز���: ��
*��������: ɾ���û������е���������
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
    /*�ҵ������û������������б�*/
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
        /*ֻɾ����ҵ�����Ϣ*/
        if(uiBidIndex == pTopTaxisPos->m_uiBidIndex)
        {
            DEBUG_LOG("UserTaxisPosList[%llu]Result[%d]", ullDelPopUin, i);
            DelTaxisTopListNode(ullDelPopUin, *pTopTaxisPos);
        }
    }
    /*���û������б�����ɾ���ú��������*/
    g_user_taxis_pos_list.erase(pMapTaxisPos);
    return 0;

}
/*************************************************************
*��������: del_taxis_pos_rule()
*�������: ��
*���ز���: ��
*��������: �����б����Ƿ�����û�����
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
    /*�ҵ������û������������б�*/
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
    /*����û������������Ѿ�û�����ݣ���ɾ��*/
    if(ui_v_size <= 0)
    {
        g_user_taxis_pos_list.erase(p_map_taxispos);
    }
    return -1;

}
/*************************************************************
*��������: del_taxis_pos_node()
*�������: ��
*���ز���: ��
*��������: ɾ��������������Ľڵ�
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
            /*����û�������������ֻ��1���ڵ㣬��ɾ�����û��ڵ�*/
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
*��������: InsertTaxisPosNode()
*�������: ��
*���ز���: ��
*��������: �����������������ڵ�
************************************************************/
int CTopTaxisHandle::InsertTaxisPosNode(unsigned long long ullNewPopUin, Toptaxis_Pos taxispos)
{
    MAP_TAXIS_POS::iterator p_map_taxispos;
    MAP_TAXIS_POS::iterator p_map_taxisposend;

    V_Toptaxis_Pos v_toptaxis_pos;
    p_map_taxispos = g_user_taxis_pos_list.begin();
    p_map_taxisposend = g_user_taxis_pos_list.end();
    /*��ϵͳ��ص����ݲ���д�����������*/
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
    /*�����ڣ�����Ϊ�½ڵ����*/
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
*��������: compare_msg_cond()
*�������: ��
*���ز���: ��
*��������: �ַ����б�����������ַ����Ƿ���ȫƥ��
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
*��������: assign_vector_list()
*�������: ��
*���ز���: ��
*��������: vector���ݸ�ֵ
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
*��������: write_water_log()
*�������: ��
*���ز���: ��
*��������: д��ˮ��־
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


    /*AB�ļ���ʽ��Ϊ�˻�ȡ���������������*/
    if(('A' != p_stTaxisShmData->m_ucABFlag)&&('B' != p_stTaxisShmData->m_ucABFlag))
    {
        ERROR_LOG("shmflag[%c]", p_stTaxisShmData->m_ucABFlag);
        return -1;
    }
    g_cCommSemp.Lock();
    /*�����������A�����ݣ���дB������*/
    if('A' == p_stTaxisShmData->m_ucABFlag)
    {
        /*�����б�*/
        for(i = 0; i < ui_keyvaluesize; i++)
        {
            p_stTaxisShmData->m_tBData[i].m_ullKey =  r_top_info.m_vKeyValueList[i].m_ullKey;
            p_stTaxisShmData->m_tBData[i].m_iValue = r_top_info.m_vKeyValueList[i].m_iValue;
            p_stTaxisShmData->m_tBData[i].m_iChangeValue = r_top_info.m_vKeyValueList[i].m_iChangeValue;
        }
        p_stTaxisShmData->m_uiBNum = ui_keyvaluesize;
        p_stTaxisShmData->m_ucABFlag = 'B';
    }
    else/*�����������B������,��дA������*/
    {
        /*�����б�*/
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
*��������: write_water_log()
*�������: ��
*���ز���: ��
*��������: д��ˮ��־
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
    /*����ֵ�б�*/
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
    /*�����б�*/
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
    /*�Ƿ���Ҫ�乲���ڴ�*/
    if(E_TAXIS_COLD_WATERLOG != i_cold_flag)
    {
        /*��ǰ�Ƿ��ڴ�����Ϣ��־, ����ÿ����Ϣֻ��¼һ����ˮ*/
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
*��������: create_toplist_file(void)
*�������: ��
*���ز���: ��
*��������: ������¼top������ļ��������쳣����µ����ݻָ�
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

    /*attach �����ڴ�*/
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
    /*�����ڴ��keyд���ļ�*/
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
*��������: handle_msg()
*�������: ��
*���ز���: ��
*��������: �����DC_Agent������ͬ������
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
    /*ҵ��ID�����ڣ�����ʧ��*/
    if(g_uiBidNum == ui_BidIndex)
    {
        ERROR_LOG("BidNum[%u]Bid[%u]Code[NotExist]", g_uiBidNum, toptaxis_msg.m_uiBid);
        return -1;
    }

    /*�û����ݵ�ɾ����ֱ��ɾ��*/
    if((E_TAXIS_MSG_OPT_DEL == toptaxis_msg.m_usOptType)&&(E_TAXIS_DATA_TYPE_USER == toptaxis_msg.m_usDataType))
    {
        DelUinAllTaxis(toptaxis_msg.m_ullNumber, ui_BidIndex);
        DEBUG_LOG("DelUinAllTaxis[OK]");
        return 0;
    }
    
    /*�������������Ϣ������ֱ�ӷ���*/
    if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vWhiteList, toptaxis_msg.m_ullNumber))
    {
        INFO_LOG("WhiteList[%llu]Pos[BidLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
        return 0;
    }

    
    uiMsgConditionNum = toptaxis_msg.m_vConditionInfo.size();
    uiConfConditionNum = g_condition_list[ui_BidIndex].m_vConditionList.size();
    /*���������ļ��е��������ҵ���Ϣ����֮ƥ�������*/
    /*�����ļ��е������б�*/
    for(ui_ConditionIndex = 0; ui_ConditionIndex < uiConfConditionNum; ui_ConditionIndex++)
    {
        uiTmp = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vCondition.size();
        /*�������������Ϣ������ֱ�ӷ���*/
        if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vWhiteList, toptaxis_msg.m_ullNumber))
        {
            INFO_LOG("WhiteList[%llu]Pos[CondLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
            continue;
        }
        v_value_list.clear();
        /*�����������Ҫ������һ��ƥ��*/
        if(0 != uiTmp)
        {
            if(0 == compare_msg_cond(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vCondition, 
                toptaxis_msg.m_vConditionInfo))
            {
                //v_value_list.clear();
                /*����Ϣ���������Ե�ֵ��������*/
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
        /*�����ļ�������ÿһ������������Ϣ���涼���ڣ����������
        ���������������κ���������ֱ��ƥ������������*/
        /*����ֵ�����仯��������з����仯,��ɾ��������*/
        if(0 != toptaxis_msg.m_usCondChange)
        {
            /*ϵͳ������ݲ���������ֵ�����仯�������Ʊ�Ͳ��ܷ������б仯���̶���һ������*/
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
        /*���������ļ�����ĳ���������й�������*/
        for(ui_RuleIndex = 0; ui_RuleIndex < uiTmp3; ui_RuleIndex++)
        {
            /*���������Ч����Ҳ����������*/
            if(E_RULE_DISABLE == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_cEnableFlag)
            {
                INFO_LOG("BidIndex[%u]ConditionIndex[%u]RuleIndex[%u]Result[RULEDISABLE]", ui_BidIndex, ui_ConditionIndex, ui_RuleIndex);
                continue;
            }
            /*�������������Ϣ������ֱ�ӷ���*/
            if(0 == FindWhiteList(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vWhiteList, 
                toptaxis_msg.m_ullNumber))
            {
                INFO_LOG("WhiteList[%llu]Pos[RuleLevel]Result[NoHandle]", toptaxis_msg.m_ullNumber);
                continue;
            }
            for(q = 0; q < uiTmp4; q++)
            {
                /*�����ļ��Ĺ�������Ϣ����(����)ƥ��һ�£����������*/
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
                            /*��Ҫ���浱ǰֵ����������*/
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

                            /*�������� ������������������*/
                            iRet = VectorUndataSort(t_sync_toptaxis.m_taxispos,
                                toptaxis_msg.m_vFieldInfo[q].m_uiValue,
                                iDataTemp,
                                toptaxis_msg.m_ullNumber,
                                g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_uiTopNum, 
                                g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_usTaxisType,
                                ullPopUinTmp, t_posinfo);
                            /*ԭ����������е�����������ˮ����ͬ��*/
                            if(0 == iRet)
                            {
                                DEBUG_LOG("BidIndex[%u]ConditionList[%u]RuleList[%u]TopList[%u]Code[SortData]", 
                                    ui_BidIndex, ui_ConditionIndex, ui_RuleIndex, ui_TopListIndex);
                                /*���а������û��°���ɾ���û�����������. ϵͳ��ص���������ɾ��*/
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
                                /*д��ˮ��־*/
                                WriteWaterLog(&t_sync_toptaxis,
                                    g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_vTopList[ui_TopListIndex],
                                    E_TAXIS_REAL_WATERLOG);
                            }
                            break;
                        }
                    }
                    /*���һ�����������ڻ��������������´����µ�����*/
                    if(ui_TopListIndex == uiTmp1)
                    {
                        Top_Info tmp_topinfo;
                        Key_Value tmp_keyvalue;
                        tmp_keyvalue.m_iValue = toptaxis_msg.m_vFieldInfo[q].m_uiValue;
                        /*��Ҫ���浱ǰֵ����������*/
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
                        /*д��ˮ��־*/
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
* Description: ��ϵͳ���ù�����������һЩ�¹���
*       Input: 
*              
*      Output: 
*      Return: 0 �ɹ� ����ֵʧ��
*      Others: ��
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
    /*���������ļ��е��������ҵ���Ϣ����֮ƥ�������*/
    /*�����ļ��е������б�*/
    for(uiCondIndex = 0; uiCondIndex < uiConfConditionNum; uiCondIndex++)
    {
        Condition_Info &rCondInfo = g_condition_list[uiBidIndex].m_vConditionList[uiCondIndex];  
        if(usCondId == rCondInfo.m_usCondId)
        {
            INFO_LOG("WhiteListBeginReload[CondLevel]CondId[%u]", usCondId);
            /*��������Ҫ���µ���*/
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
                /*�ù��������еĹ���ID�ظ�������Ҫ�ظ���ӣ�ֻ��Ҫ���°���������(ǰ��������Ѿ����¹���)*/
                if(0 == uiRuleId)
                {
                    continue;
                }
                tRuleTopTaxisPos.m_uiConditionListIndex = uiCondIndex;
                tRuleTopTaxisPos.m_uiRuleListIndex = rCondInfo.m_vRuleList.size();
                
                INFO_LOG("RuleReloadOldCondInsert[%u]Bid[%u]Cond[%u]Rule[%u]", uiRuleId, tRuleTopTaxisPos.m_uiBidIndex, 
                    tRuleTopTaxisPos.m_uiConditionListIndex, tRuleTopTaxisPos.m_uiRuleListIndex);
                /*ϵͳ���еĹ�������*/
                gMapRuleList.insert(make_pair(uiRuleId, tRuleTopTaxisPos));                
                rCondInfo.m_vRuleList.push_back(stCondInfo.m_vRuleList[uiRuleIndex]);
            }
            INFO_LOG("RuleOldCondEndReload[RuleLevel]BidIndex[%u]", uiBidIndex);
            return 0;
        }
    }
    tRuleTopTaxisPos.m_uiConditionListIndex = g_condition_list[uiBidIndex].m_vConditionList.size();
    uiConfRuleNum = stCondInfo.m_vRuleList.size();
    /*���¹�������*/
    for(uiRuleIndex = 0; uiRuleIndex < uiConfRuleNum; uiRuleIndex++)      
    {
        tRuleTopTaxisPos.m_uiRuleListIndex = uiRuleIndex;
        uiRuleId = stCondInfo.m_vRuleList[uiRuleIndex].m_uiRuleId;
        INFO_LOG("RuleReloadNewCondInsert[%u]Bid[%u]Cond[%u]Rule[%u]", uiRuleId, tRuleTopTaxisPos.m_uiBidIndex, 
            tRuleTopTaxisPos.m_uiConditionListIndex, tRuleTopTaxisPos.m_uiRuleListIndex);
        /*ϵͳ���еĹ�������*/
        gMapRuleList.insert(make_pair(uiRuleId, tRuleTopTaxisPos));                
    }
    g_condition_list[uiBidIndex].m_vConditionList.push_back(stCondInfo);
    INFO_LOG("RuleNewCondEndReload[RuleLevel]BidIndex[%u]", uiBidIndex);
    return 0;    
}
/*******************************************************************************
* Description: ���µ�ǰ��ϵͳ���ù���
*       Input: 
*              
*      Output: 
*      Return: 0 �ɹ� ����ֵʧ��
*      Others: ��
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
                /*��������Ҫ���µ���*/
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
                /*��������ѭ��*/
                ucOKFlag = 1;
                break;
            }
        }
        if(1 == ucOKFlag)
        {
            break;
        }
    }
    /*��ҵ��ID�������Ϊ��ҵ��*/
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
* Description: ɨ���ȡָ��Ŀ¼���������xml�ļ�
*       Input: strDir ָ����Ŀ¼·��
*              
*      Output: rFileList ָ��Ŀ¼·��������xml�ļ�������·���б�
*      Return: 0 �ɹ� ����ֵʧ��
*      Others: ��
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
* Description: ɨ���ȡָ��Ŀ¼��������б仯��xml�ļ�
*       Input: strDir ָ����Ŀ¼·��
*              
*      Output: rFileList ָ��Ŀ¼·�������з����仯��xml�ļ�����·���б�
*      Return: 0 �ɹ� ����ֵʧ��
*      Others: ��
*******************************************************************************/
int ScanDirXmlFile(const char* strDir, vector<string> &rFileList)
{
    /*��ȡ�޸ĵ������ļ�*/
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

    /*��ȡ�¼������ļ�*/
    return GetDirXmlFile(strDir, rFileList);
}
/*******************************************************************************
* Description: ɨ����������ļ��ĸĶ��������ж�̬����
*       Input: 
*              
*      Output: 
*      Return: 0 �ɹ� ����ֵʧ��
*      Others: ��
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
        /*�Ѵ��ڵĹ��������ļ�*/
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
        /*�¹��������ļ�����Ҫ���������ļ���bid��ӳ���ϵ*/
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

