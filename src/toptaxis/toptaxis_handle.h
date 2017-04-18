/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_handle.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP排序系统处理模块的函数声明
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_HANDLE_H_
#define _TOPTAXIS_HANDLE_H_
#include <vector>
#include <string>
#include <map>
#include <set>
#include "tcp_client.h"

using namespace std;

#define RULE_FILE_NAME_LEN 30

class CTopTaxisHandle
{
public:
    CTopTaxisHandle(){};
    virtual ~CTopTaxisHandle(){};
    virtual int InsertTaxisPosNode(unsigned long long ullNewPopUin, Toptaxis_Pos taxispos);
    virtual int HandleMsg(Toptaxis_Msg& toptaxis_msg);
    virtual int DelTaxisPosNode(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos);
    virtual int DelTaxisPosRule(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos);
    virtual int DelTaxisTopListNode(unsigned long long ullDelPopUin, Toptaxis_Pos taxispos);
    virtual int UpdateCurData(MAP_HISTORY_VALUE& map_hostory, unsigned int uiValue, unsigned long long ullUin, int* iDiffData, FILE* pFd, 
    unsigned short usTimeType);
    virtual int VectorUndataCompare(vector<unsigned int> &v_uiListOrg, vector<unsigned int> &v_uiListDst);
    virtual int VectorUndataSort(Toptaxis_Pos &r_toptaxis_pos, int iCurData, int uiData, unsigned long long ullUin, unsigned int uiSortNum, 
    unsigned short usTaixType, unsigned long long &ullPopUin, Sort_Pos_Info &r_posinfo);
    virtual int WriteTopShm(Top_Info& r_top_info);
    virtual int WriteWaterLog(Sync_Toptaxis *p_sync_toptaxis, Top_Info& r_top_info, int i_cold_flag);
    virtual int CreateToplistFile(Top_Info& r_top_info, Toptaxis_Pos t_toptaxis_pos);
    virtual int DelUinAllTaxis(unsigned long long ullDelPopUin, unsigned int uiBidIndex);  
    virtual int FindWhiteList(V_WHITE_LIST &v_whitelist, unsigned long long ullIn);  
};

int AddCondRule(unsigned int uiBidIndex, Condition_Info& stCondInfo);
int UpdateSysRuleConf(void);
int GetDirXmlFile(const char* strDir, vector<string>& rFileList);
int ScanDirXmlFile(const char* strDir, vector<string>& rFileList);
int ScanRuleSysConf(void);

#endif


