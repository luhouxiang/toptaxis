/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _DC_PROCESS_H_
#define _DC_PROCESS_H_

#include<vector>
#include<string>
#include<map>
#include<set>

#include "bitservice.h"
#include "toptaxis_header.h"

extern map<unsigned int, set<string> > g_cond_list; /*系统中所有条件列表*/
extern map<unsigned int, set<string> > g_field_list; /*系统中所有条件列表*/
/*每处理100万条消息统计一下性能*/
#define TAXIS_HANDLE_COUNT                         1000000

using namespace BitService;
using namespace std;


class CDataProcess
{
public:
    CDataProcess();
    ~CDataProcess();

    static CDataProcess* Instance(); 


    int Init();
    int Decode(const char* pszData,int iLen);
    int SetTopMsg();
    int Process();
    string GetReqMsg();


private:
    int GetStringValue(string strKey,string& strValue);
    int GetIntValue(string strKey,int& iValue);
    int GetUnIntValue(string strKey,unsigned& uiValue);
    int GetULLValue(string strKey, unsigned long long& ullValue );

    int CheckReq();
    int ParseModify(string& strMValue);
    int DeleteStore(vector<string>& vAttrOp,unsigned short& usBid);
    int InsertStore(vector<string>& vAttrOp,unsigned short& usBid);
    int UpdateStore(vector<string>& vAttrOp,unsigned short& usBid);	
    int SetStore(vector<string>& vAttrOp,unsigned short& usBid);

private:
    static CDataProcess * m_Instance;

    map<unsigned short,CBitService*> m_mBid2Server;
    map<string,string> m_mKey2Value;
    map<string,vector<string> > m_mAttr2Op;

    unsigned long long m_ullUin;
    unsigned m_uiUin;
    unsigned m_uiAppID;
    string m_strReq;

public:
    Toptaxis_Msg m_Top;
};



#endif


