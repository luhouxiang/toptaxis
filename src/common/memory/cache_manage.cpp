/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       �ļ�����:  cache_manage.cpp
 *       �ļ�����:  ������ ����ͬ���������ѯ����
                    1: �����򱸻����մ�����ͬ������������
                    2: ������ǰ�˲�ѯ����ӿ�
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.09.25 
 *       �޸ļ�¼:  
 ***************************************************************************/
 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <vector>

#include "strutil.h"
#include "cache_manage.h"
#include "log.h"

#include "timestamp.h"

#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "toptaxis_conf.h"
#include "toptaxis_backup.h"
#include "toptaxis_sync.h"
#include "toptaxis_handle.h"
#include "cw_stock_map.h"
#include "stringExt.h"
#include "vectorExt.h"

/*ϵͳ�����������б�*/
extern vector <Bid_Condition> g_condition_list;
extern CTaxisServiceConf * g_pCServiceConf;
/*ϵͳ��ҵ����Ŀ*/
extern unsigned int g_uiBidNum; 
extern unsigned int g_ui_begin_time;

/*ȫ�ִ���������Ϣ�Ķ���*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

CCacheManage::CCacheManage()
{

}

CCacheManage::CCacheManage(int iStartKey, unsigned int uiDataCacheSize, char* pWaterLogPath)
{

}

CCacheManage::~CCacheManage()
{
}
/*************************************************************
*��������: ProcessQryData(bool bFlag)
*�������: pReqBody:�������   pRspBody:��Ӧ����  uiResponseLen:��Ӧ����
*���ز���: 0:������Ϣ�� -1:��Ҫ�ϲ���ú���ֱ�Ӷϵ���·
*��������: ������˵Ĳ�ѯ����
************************************************************/
int ProcessQryData(const char * pReqBody, char * pRspBody, unsigned int& uiResponseLen)
{
    Query_Taxis_Req * pQueryDataReq = (Query_Taxis_Req * )pReqBody;
    Query_Taxis_Rsp * pQueryDataRsp = (Query_Taxis_Rsp * )pRspBody;
    Taxis_Query_Body_Rsp * pQueryBodyRsp = NULL;
    unsigned int ui_ConditionIndex = 0, ui_BidIndex = 0, ui_TopListIndex = 0, ui_RuleIndex = 0, i =0;
    unsigned int uiConfConditionNum = 0, uiRuleNum = 0, uiTopNum = 0;
    unsigned int uiTopKeyNum = 0, uiTopDisplayNum = 0;
    unsigned int uiCondValueTmp = 0, uiSerId = 0;
    vector <unsigned int> v_value_list;
    Top_Info *p_top_info = NULL;
    Rule_Info *p_rule_info = NULL;

    if((NULL == pReqBody)||(NULL == pRspBody))
    {
        ERROR_LOG("pReqBody[NULL]pRspBody[NULL]");
        return -1;
    }

    pQueryBodyRsp = pQueryDataRsp->m_szMsgBody;

    pQueryDataRsp->m_sResultCode = ERROR_DATA_NOT_EXIT;
    uiResponseLen = BITMAP_HEADER_LEN + sizeof(Query_Taxis_Rsp);

    for(ui_BidIndex = 0; ui_BidIndex < g_uiBidNum; ui_BidIndex++)
    {
        if(g_condition_list[ui_BidIndex].m_uiBid == pQueryDataReq->m_usBID)
        {
            break;
        }
    }
    /*ҵ��ID�����ڣ��򷵻�*/
    if(g_uiBidNum == ui_BidIndex)
    {
        ERROR_LOG("CMD[QueryData]BidNum[%u]ReqBidId[%u]", g_uiBidNum, pQueryDataReq->m_usBID);
        return 0;
    }
    uiConfConditionNum = g_condition_list[ui_BidIndex].m_vConditionList.size();
    /*���������ļ��е��������ҵ���Ϣ����֮ƥ�������*/
    /*�����ļ��е������б�*/
    for(ui_ConditionIndex = 0; ui_ConditionIndex < uiConfConditionNum; ui_ConditionIndex++)
    {
        if(pQueryDataReq->m_usCondId  == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_usCondId)
        {
            break;
        }
    }
    /*����ID�����ڣ��򷵻�*/
    if(uiConfConditionNum == ui_ConditionIndex)
    {
        ERROR_LOG("CMD[QueryData]CondNum[%u]ReqCondId[%u]", uiConfConditionNum, pQueryDataReq->m_usCondId);
        return 0;
    }
    uiSerId = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_uiSerId;
    uiRuleNum = g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList.size();

    for(ui_RuleIndex = 0; ui_RuleIndex < uiRuleNum; ui_RuleIndex++)
    {
        if(pQueryDataReq->m_uiRuleId == g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex].m_uiRuleId)
        {
            break;
        }
    }
    /*����ID�����ڣ��򷵻�*/
    if(uiRuleNum == ui_RuleIndex)
    {
        ERROR_LOG("CMD[QueryData]BidIndex[%u]ConditionList[%u]RuleNum[%u]ReqRuleId[%u]RuleId0[%u]", 
            ui_BidIndex, ui_ConditionIndex, uiRuleNum, pQueryDataReq->m_uiRuleId
            ,g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[0].m_uiRuleId);
        return 0;
    }
    p_rule_info = (Rule_Info*)&(g_condition_list[ui_BidIndex].m_vConditionList[ui_ConditionIndex].m_vRuleList[ui_RuleIndex]);
    for(i = 0; i < pQueryDataReq->m_usValueNum; i++)
    {
        uiCondValueTmp = *((unsigned int*)(pQueryDataReq->m_uiCondValue)+i);
        v_value_list.push_back(uiCondValueTmp);
        DEBUG_LOG("PushCondValueIndex[%u]Value[%u]", i, uiCondValueTmp);
    }

    uiTopNum = p_rule_info->m_vTopList.size();
    for(ui_TopListIndex = 0; ui_TopListIndex < uiTopNum; ui_TopListIndex++)
    {
        /*����ֵ��ȫƥ�������*/
        if(0 == g_pCTopTaxisHandle->VectorUndataCompare(v_value_list, 
            p_rule_info->m_vTopList[ui_TopListIndex].m_vUiConditionValue))
        {
            p_top_info = (Top_Info*)&(p_rule_info->m_vTopList[ui_TopListIndex]);
            uiTopKeyNum = p_top_info->m_vKeyValueList.size();
            if(uiTopKeyNum < p_rule_info->m_uiTopDisplayNum)
            {
                uiTopDisplayNum = uiTopKeyNum;
            }
            else
            {
                uiTopDisplayNum = p_rule_info->m_uiTopDisplayNum;
            }
            for(i = 0; i < uiTopDisplayNum; i++)
            {
                pQueryBodyRsp = pQueryDataRsp->m_szMsgBody + i;
                {
                    pQueryBodyRsp->m_ullKey= p_top_info->m_vKeyValueList[i].m_ullKey;
                }
                pQueryBodyRsp->m_iValue = p_top_info->m_vKeyValueList[i].m_iValue; 
                pQueryBodyRsp->m_iChange = p_top_info->m_vKeyValueList[i].m_iChangeValue; 
            }
            uiResponseLen += sizeof(Taxis_Query_Body_Rsp)*uiTopDisplayNum;
            /*��Ϣ���Ȳ���Խ��*/
            if(uiResponseLen >= MAX_RESPONSE_MSG_LEN-1)
            {
                pQueryDataRsp->m_sResultCode = ERROR_DATA_LEN_EXCEED;
                pQueryDataRsp->m_usKeyNum = 0;
                uiResponseLen -= sizeof(Taxis_Query_Body_Rsp)*uiTopDisplayNum;
                ERROR_LOG("CMD[QueryData]ResponseLen[%u]maxResponseLen[%u]", uiResponseLen, MAX_RESPONSE_MSG_LEN);
                return 0;
            }
            pQueryDataRsp->m_sResultCode = SUCCESS_CODE;
            pQueryDataRsp->m_usKeyNum = uiTopDisplayNum;
            pQueryDataRsp->m_uiSerId = uiSerId;
            return 0;
        }
    }
    /*����ID���治��������*/
    if(uiTopNum == ui_TopListIndex)
    {
        ERROR_LOG("CMD[QueryData]BidIndex[%u]ConditionList[%u]RuleIndex[%u]RuleId[%u]TopNum[%u]Nolist", 
            ui_BidIndex, ui_ConditionIndex, ui_RuleIndex, pQueryDataReq->m_uiRuleId, uiTopNum);
    }
    return 0;
}
/*************************************************************
*��������: ProcessReplaceData(bool bFlag)
*�������: pReqBody:�������   pRspBody:��Ӧ����  uiResponseLen:��Ӧ����
*���ز���: 0:������Ϣ�� -1:��Ҫ�ϲ���ú���ֱ�Ӷϵ���·
*��������: ������˵ĸ��ǲ���,��������������ͬ��
************************************************************/
int CCacheManage::ProcessReplaceData(const char * pReqBody, char * pRspBody, unsigned int* uiResponseLen)
{
    Replace_Data_Req* pReplaceDataReq = (Replace_Data_Req * )pReqBody;
    Replace_Data_Rsp * pReplaceDataRsp = (Replace_Data_Rsp * )pRspBody;

    unsigned int  uiFile =  pReplaceDataReq->m_uiFileName;
    unsigned int  uiLine =  pReplaceDataReq->m_uiLineNum;
    unsigned long  ulOffSet =  pReplaceDataReq->m_ulOffSet;

    unsigned int  uiOutFile    = 0;
    unsigned int  uiOutLine    = 0;
    unsigned long  ulOutOffSet = 0;

    char * pTemp = (char *)pReplaceDataReq->m_struData;
    Replace_Base_Data *pReplaceBody;
    unsigned short  usTemp;


    /*Ĭ�Ͻ��ֵΪ�ɹ�*/
    pReplaceDataRsp->m_sResultCode = SUCCESS_CODE;
    /*�����Ӧ��Ϣ����*/
    *uiResponseLen = BITMAP_HEADER_LEN + sizeof(Replace_Data_Rsp);

    /*ѭ������replace ����*/
    for(usTemp = 0; usTemp < pReplaceDataReq->m_usRecordNum; usTemp++)
    {
        uiOutFile = uiFile;
        uiOutLine = uiLine;
        ulOutOffSet = ulOffSet;

        pReplaceBody = (Replace_Base_Data *)pTemp;

        pReplaceDataRsp->m_sResultCode = ProcessSingleReplace(pReplaceDataReq->m_szHardSync, pReplaceBody->m_szDataBuf, 
            pReplaceBody->m_usDataLen,uiOutFile,uiOutLine,ulOutOffSet);

        /*����м䴦���������������ļ����ƣ��кţ�ƫ����ֱ�ӷ���*/
        if(pReplaceDataRsp->m_sResultCode)
        {
            pReplaceDataRsp->m_uiFileName = uiOutFile;
            pReplaceDataRsp->m_uiLineNum = uiOutLine;
            pReplaceDataRsp->m_ulOffSet = ulOutOffSet;	
            return 0;
        }

        /*�޸���һ����Ϣ���кţ�ƫ����*/
        uiLine++;
        ulOffSet += pReplaceBody->m_usDataLen;

        /*�ҵ���һ��ͬ����Ϣ����Ϣ��ָ��*/
        pTemp += sizeof(Replace_Base_Data) + pReplaceBody->m_usDataLen;

    }
    pReplaceDataRsp->m_uiFileName = uiOutFile;
    pReplaceDataRsp->m_uiLineNum = uiOutLine;
    pReplaceDataRsp->m_ulOffSet = ulOutOffSet;	

    return 0;
}

/*************************************************************
*��������: ProcessDelData(bool bFlag)
*�������: pReqBody:�������   pRspBody:��Ӧ����  uiResponseLen:��Ӧ����
*���ز���: 0:�����ɹ� pRspBody[0] = '0' �ɹ�ɾ��  pRspBody = [0] = '1' ɾ��Ԫ�ز�����
*��������: ������˵�ɾ������
************************************************************/
int CCacheManage::ProcessDelData(const char * pReqBody, char * pRspBody, unsigned int* uiResponseLen)
{
    /*Ĭ�ϲ���ʧ�ܣ����ذ�����Ϊ1*/
    pRspBody[0] = TAXIS_DEL_FAIL;
    *uiResponseLen = TAXIS_DEL_RSP_LEN;

    if(NULL == pReqBody)
    {
        return -1;
    }

    /*�������ת��Ϊmap*/
    map<string, string> mReqMsg;
    StrExt::str2Map(pReqBody, mReqMsg, "&", "=");
   
    unsigned int uin = strtoul(mReqMsg["uin"].c_str(), NULL, 0);

    /**�ж�uin�Ƿ����Ҫ��*/
    if(uin !=0 && uin <= TAXIS_MIN_UIN)
    {
        return 0;
    }

    unsigned int uiBid = strtoul(mReqMsg["bid"].c_str(), NULL, 0);
    unsigned int uiRuleId = strtoul(mReqMsg["ruleid"].c_str(), NULL, 0);
    string strCondVals = mReqMsg["condvals"];

    /*��������ֵ����������ֵת��Ϊusigned����*/
    vector<string> vstrCondVals;
    StrExt::str2Vect(strCondVals.c_str(), vstrCondVals, ",");
    vector<unsigned int> vuiCondVals;
    VectorStringToUnsigned(vstrCondVals, vuiCondVals);

    vector <Bid_Condition>::iterator itBusBegin = g_condition_list.begin();
    vector <Bid_Condition>::iterator itBusEnd = g_condition_list.end();
    vector <Bid_Condition>::iterator itBusPos;

    for(itBusPos = itBusBegin; itBusPos < itBusEnd; itBusPos++)
    {
        if(uiBid == itBusPos->m_uiBid)
        {
            vector <Condition_Info>::iterator itCondBegin = itBusPos->m_vConditionList.begin();
            vector <Condition_Info>::iterator itCondEnd = itBusPos->m_vConditionList.end();
            vector <Condition_Info>::iterator itCondPos;
            
            for(itCondPos = itCondBegin; itCondPos < itCondEnd; itCondPos++)
            {
                vector <Rule_Info>::iterator itRuleBegin = itCondPos->m_vRuleList.begin();
                vector <Rule_Info>::iterator itRuleEnd = itCondPos->m_vRuleList.end();
                vector <Rule_Info>::iterator itRulePos;
                
                for(itRulePos = itRuleBegin; itRulePos < itRuleEnd; itRulePos++)
                {
                    if(uiRuleId == itRulePos->m_uiRuleId)
                    {
                        vector <Top_Info>::iterator itTopBegin = itRulePos->m_vTopList.begin();
                        vector <Top_Info>::iterator itTopEnd = itRulePos->m_vTopList.end();
                        vector <Top_Info>::iterator itTopPos;
                        
                        for(itTopPos = itTopBegin; itTopPos < itTopEnd; itTopPos++)
                        {
                            /*�ж�����ֵ�Ƿ���ȣ��ȶ�vector������˳��Ƚ�*/
                            if( VectorCmp(itTopPos->m_vUiConditionValue, vuiCondVals, true))
                            {
                                /*ɾ�����������нڵ�*/
                                if(uin == 0)
                                {
                                    (itRulePos->m_vTopList).erase(itTopPos);
                                    pRspBody[0] = TAXIS_DEL_SUCC;

                                    return 0;
                                }
                                /*ɾ��һ���ڵ�*/
                                else
                                {
                                    vector <Key_Value>::iterator itNodeBegin = itTopPos->m_vKeyValueList.begin();
                                    vector <Key_Value>::iterator itNodeEnd = itTopPos->m_vKeyValueList.end();
                                    vector <Key_Value>::iterator itNodePos;

                                    for(itNodePos = itNodeBegin; itNodePos < itNodeEnd; itNodePos++)
                                    {
                                        if(itNodePos->m_ullKey == (unsigned long long)uin)
                                        {
                                            (itTopPos->m_vKeyValueList).erase(itNodePos);
                                            pRspBody[0] = TAXIS_DEL_SUCC;

                                            return 0;
                                        }
                                    }                                   
                                }  
                                return 0;
                            }
                        } 
                        return 0;
                    }
                }  
            }
            return 0;
        }
    }

    return 0;
}

int CCacheManage::ProcessSingleReplace(char szHard, char*pRData, unsigned short usRLen,unsigned int &uiFile,
    unsigned int & uiLine,unsigned long & ulOffSet)
{
    /*��ȡ��ʵ���ݳ���*/
    unsigned int ui_size = 0, ui_total = 0, i = 0;
    unsigned int uiBidIndex = 0, uiConditionListIndex = 0, uiRuleListIndex = 0;
    unsigned int uiTopListIndex = 0, uiConditionValueNum = 0, uiTopListNum = 0;
    unsigned int uiCondValuePos = 0, uiSysTopListSize = 0;

    unsigned int uiBidSize = 0, uiCondSize = 0, uiRuleSize = 0;
    vector<string> vKey;
    vector<unsigned int> vCondValueList;
    Top_Info t_top_info_tmp;
    Key_Value t_key_value_tmp;
    vector<string> vTemp;
    Toptaxis_Pos t_toptaxis_pos;
    bzero(&t_top_info_tmp, sizeof(t_top_info_tmp));
    vKey.clear();
    CStrUtil::SplitStringUsing(pRData, "|", &vKey);

    ui_size = vKey.size();
    if(ui_size <= TAXIS_MINNUM_WATER_LOG)
    {
        ERROR_LOG("Replacesize[%u]minsize[%u]data[%s]", ui_size, TAXIS_MINNUM_WATER_LOG, pRData);
        return -1;
    }

    uiBidIndex = (unsigned int)atoi(vKey[i++].c_str());
    uiConditionListIndex = (unsigned int)atoi(vKey[i++].c_str());
    uiRuleListIndex = (unsigned int)atoi(vKey[i++].c_str());
    uiTopListIndex = (unsigned int)atoi(vKey[i++].c_str());
    uiConditionValueNum = (unsigned int)atoi(vKey[i++].c_str());
    uiTopListNum = (unsigned int)atoi(vKey[i++].c_str());

    ui_total = TAXIS_MINNUM_WATER_LOG + uiConditionValueNum + uiTopListNum;
    if(ui_size != ui_total)
    {
        ERROR_LOG("Replacesize[%u]minsize[%u]condsize[%u]topnum[%u]data[%s]", ui_size, 
        TAXIS_MINNUM_WATER_LOG, uiConditionValueNum, uiTopListNum, pRData);
        return -1;
    }
    /*����ֵ*/
    uiCondValuePos = TAXIS_MINNUM_WATER_LOG + uiConditionValueNum;
    for(; i < uiCondValuePos; i++)
    {
        t_top_info_tmp.m_vUiConditionValue.push_back((unsigned int)atoi(vKey[i].c_str()));
    }
    /*Top����ֵ*/
    for(; i < ui_total; i++)
    {
        vTemp.clear();
        CStrUtil::SplitStringUsing(vKey[i], "_", &vTemp);
        if(TAXIS_NUM_KEY_VALUE != vTemp.size())
        {
            ERROR_LOG("CMD[ReplaceData]WaterlogIndex[%u]value[%s]", i, vKey[i].c_str());
            continue;
        }
        //t_key_value_tmp.m_uiKey = (unsigned int)atoi(vTemp[0].c_str());
        t_key_value_tmp.m_ullKey = strtoull(vTemp[0].c_str(), 0, 10);
        t_key_value_tmp.m_iValue = (unsigned int)atoi(vTemp[1].c_str());
        t_key_value_tmp.m_iChangeValue = (unsigned int)atoi(vTemp[2].c_str());
        t_top_info_tmp.m_vKeyValueList.push_back(t_key_value_tmp);
    }

    uiBidSize = g_condition_list.size();
    if(uiBidIndex >= uiBidSize)
    {
        ERROR_LOG("CMD[ReplaceData]OverLoad[!!!!!]BidSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]", 
            uiBidSize, 
            uiBidIndex, uiConditionListIndex, uiRuleListIndex);
        return 0;
    }
    Bid_Condition& rstBidTmp = g_condition_list[uiBidIndex];
    uiCondSize = rstBidTmp.m_vConditionList.size();
    if(uiConditionListIndex >= uiCondSize)
    {
        ERROR_LOG("CMD[ReplaceData]OverLoad[!!!!!]CondSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]",
        uiCondSize,
        uiBidIndex, uiConditionListIndex, uiRuleListIndex);
        return 0;
    }
    Condition_Info& rstCondTmp = rstBidTmp.m_vConditionList[uiConditionListIndex];
    uiRuleSize = rstCondTmp.m_vRuleList.size();
    if(uiRuleListIndex >= uiRuleSize)
    {
        ERROR_LOG("CMD[ReplaceData]OverLoad[!!!!!]RuleSize[%u]BidIndex[%u]CondIndex[%u]RuleIndex[%u]", 
        uiRuleSize, 
        uiBidIndex, uiConditionListIndex, uiRuleListIndex);
        return 0;
    }
    Rule_Info& rstRuleTmp = rstCondTmp.m_vRuleList[uiRuleListIndex];
        
    vector <Top_Info> &v_TopList = rstRuleTmp.m_vTopList;
    uiSysTopListSize = v_TopList.size();

    t_toptaxis_pos.m_uiBidIndex = uiBidIndex;
    t_toptaxis_pos.m_uiConditionListIndex = uiConditionListIndex;
    t_toptaxis_pos.m_uiRuleListIndex = uiRuleListIndex;
    t_toptaxis_pos.m_uiTopListIndex = uiTopListIndex;
    /*���򲻴��ڣ�������*/
    if(uiSysTopListSize < uiTopListIndex+1)
    {
        INFO_LOG("CMD[ReplaceData]Waterlog[AddNewNode]BidIndex[%u]ConditionListIndex[%u]RuleListIndex[%u]TopListIndex[%u]", 
        uiBidIndex, uiConditionListIndex, uiRuleListIndex, uiSysTopListSize);
        g_pCTopTaxisHandle->CreateToplistFile(t_top_info_tmp, t_toptaxis_pos);
        g_pCTopTaxisHandle->WriteTopShm(t_top_info_tmp);
        v_TopList.push_back(t_top_info_tmp);
        return 0;
    }
    DEBUG_LOG("CMD[ReplaceData]Waterlog[ModNode]BidIndex[%u]ConditionListIndex[%u]RuleListIndex[%u]TopListIndex[%u]", 
        uiBidIndex, uiConditionListIndex, uiRuleListIndex, uiTopListIndex);
    /*������ڣ���ɾ���������*/
    t_top_info_tmp.m_pDataAddr = v_TopList[uiTopListIndex].m_pDataAddr;
    t_top_info_tmp.m_lasttopfd = v_TopList[uiTopListIndex].m_lasttopfd;
    v_TopList.erase(v_TopList.begin()+uiTopListIndex);
    g_pCTopTaxisHandle->WriteTopShm(t_top_info_tmp);
    v_TopList.insert(v_TopList.begin()+uiTopListIndex, t_top_info_tmp);

    return SUCCESS_CODE;
}

/* *******************************************************************
 * ************************ end for backup module ********************
 * *******************************************************************/

