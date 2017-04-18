/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include <arpa/inet.h>
#include <ctime>
#include <cstdlib>
#include "comm_tools.h"
#include "strutil.h"
#include "dc_process.h"
#include "proto_head.h"
#include "log.h"
#include "error_code.h"
#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "toptaxis_conf.h"
#include "toptaxis_backup.h"
#include "toptaxis_sync.h"
#include "toptaxis_handle.h"
#include "cw_stock_map.h"

using namespace BitService;
#define STOCK_FEILD "3.2.0"
#define TIME_FEILD "ts"
#define SOURCE "datasource"

//extern std::map< uint64_t, unsigned > mapToUnsigned;
//extern std::map< unsigned, uint64_t > mapToUint64;
//const short ERROR_DATA_NOT_EXIT= -2003;
#pragma pack(1)
struct Proto
{
    unsigned uiAttrValue;
    unsigned uiOldRank;
    unsigned uiNewRank;
};
#pragma pack()


CDataProcess* CDataProcess::m_Instance = NULL;
/*ȫ�ִ���������Ϣ�Ķ���*/
extern CTopTaxisHandle* g_pCTopTaxisHandle;

extern CRuleConf* g_pCRuleConf;

/*��ǰ�Ƿ��ڴ�����Ϣ��־, ����ÿ����Ϣֻ��¼һ����ˮ*/
extern unsigned int g_uiHandMsgFlag;

/*ͳ��Toptaxis��ǰ���ܸ��ص�ʱ�������*/
extern time_t g_tCountBeginTime;

/*************************************************************
*��������: CDataProcess()
*�������: 
*���ز���:  
*��������: ���캯������ʼ������
************************************************************/
CDataProcess::CDataProcess()
{
}

/*************************************************************
*��������: ~CDataProcess()
*�������: 
*���ز���:  
*��������: ��������
************************************************************/
CDataProcess::~CDataProcess()
{
   
}



/*************************************************************
*��������: GetStringValue()
*�������: strKey: name of the request field   
                             strValue:  value of the request field   
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: get the value of the request field
************************************************************/
int CDataProcess::GetStringValue(string strKey, string& strValue )
{
    map<string, string>::iterator iter = m_mKey2Value.find( strKey);
    if ( m_mKey2Value.end() == iter )
    {
        return -1;//û������ֶ�
    }

    strValue=iter->second;
    return 0;
}

/*************************************************************
*��������: GetIntValue()
*�������: strKey: name of the request field   
                             iValue:  value of the request field   
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: get the value of the request field
************************************************************/
int CDataProcess::GetIntValue(string strKey, int& iValue )
{
    iValue = 0;
    map<string, string>::iterator iter = m_mKey2Value.find( strKey );
    if ( m_mKey2Value.end() == iter )
    {            
        return -1;//û������ֶ�
    }

    if ( iter->second == "" )
    { 
        return -1;
    }
    const char *pTemp = iter->second.c_str();
    if(pTemp != NULL)
    {
        if('-' == pTemp[0])
        {
            //�������δ���
            iValue = atoi( iter->second.c_str());
        }
        else
        {
            //�����޷������δ���
            iValue = (unsigned int) strtoul( iter->second.c_str() ,0,10);
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

/*************************************************************
*��������: GetUnIntValue()
*�������: strKey: name of the request field   
                             uiValue:  value of the request field   
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: get the value of the request field
************************************************************/
int CDataProcess::GetUnIntValue(string strKey, unsigned& uiValue )
{
    uiValue = 0;
    map<string, string>::iterator iter = m_mKey2Value.find( strKey );
    if ( m_mKey2Value.end() == iter )
    {
        return -1;//û������ֶ�
    }

    if ( iter->second == "" )
    { 
        return -1;
    }
    const char *pTemp = iter->second.c_str();
    if(pTemp != NULL)
    {
        if('-' == pTemp[0])
        {
            //�������δ���
            uiValue = atoi( iter->second.c_str());
        }
        else
        {
            //�����޷������δ���
            uiValue = (unsigned int) strtoul( iter->second.c_str() ,0,10);
        }
    }
    else
    {
        return -1;
    }

    return 0;
}


/*************************************************************
*��������: GetULLValue()
*�������: strKey: name of the request field   
                             ullValue:  value of the request field   
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: get the value of the request field
************************************************************/
int CDataProcess::GetULLValue(string strKey, unsigned long long& ullValue )
{
    ullValue = 0;
    map<string, string>::iterator iter = m_mKey2Value.find( strKey );
    if ( m_mKey2Value.end() == iter )
    {
        return -1;//û������ֶ�
    }

    if ( iter->second == "" )
    { 
        return -1;
    }

    ullValue = strtoull( iter->second.c_str() ,0,10);

    return 0;
}



/*************************************************************
*��������: Instance()
*�������: 
*���ز���: ��ʵ��ָ��
*��������: ����һ�����ʵ��   
************************************************************/
CDataProcess* CDataProcess::Instance()
{
    if ( NULL == m_Instance )
    {
        m_Instance = new CDataProcess;
    }
    return m_Instance;
}


int CDataProcess::Init()
{
    map<unsigned short,stL5Info>::iterator it;

    stL5Info L5Temp;
    CBitService* pBit=NULL;
    
    for(it = g_pCRuleConf->m_mBid2L5.begin(); it != g_pCRuleConf->m_mBid2L5.end(); ++it)
    {
        pBit=new CBitService;
        if(NULL==pBit)
        {
            ERROR_LOG("Init new null error");
            return -1;
        }

        L5Temp=it->second;
        if(pBit->Init(L5Temp.iModId,L5Temp.iCmd,0,L5Temp.uiTimeOut))
        {
            ERROR_LOG("CBitService Init error");
            delete pBit;
            pBit=NULL;
            return -1;
        }

        if(m_mBid2Server.find(it->first) != m_mBid2Server.end() && (NULL != m_mBid2Server[it->first]))
        {
            delete m_mBid2Server[it->first];
        }

        m_mBid2Server[it->first]=pBit;
    }

    return 0;
}


int CDataProcess::Decode(const char* pszData,int iLen)
{
    if(iLen<(int)sizeof(ProHdr))
    {
        ERROR_LOG("Decode error all[%d] less than head[%d]",iLen,sizeof(ProHdr));
        return -1;
    }

    DEBUG_LOG("Decode data len[%d]",iLen);
    ProHdr* Header=(ProHdr*)pszData;

    int iDataLen=ntohs(Header->usLen);
    DEBUG_LOG("Decode version[%d] data len[%d]",Header->ucVersion,iDataLen);
    if(((int)sizeof(ProHdr)+iDataLen)!=iLen)
    {
        ERROR_LOG("Decode error all[%d]data[%d]head[%d]",iLen,iDataLen,sizeof(ProHdr));
        return -1;
    }

    string strData(pszData+sizeof(ProHdr),iDataLen);
    NOTI_LOG("Decode data[%s]",strData.c_str());

    strData+="&";
    TextToMap(strData.c_str(),"&",m_mKey2Value);	

    if(CheckReq())
    {
        return -1;
    }

    return 0;
}

int CDataProcess::CheckReq()
{
    unsigned long long ullUin=0;
    if(GetULLValue(g_pCRuleConf->m_strUinName,ullUin))
    {
        ERROR_LOG("key[%s] not in req", g_pCRuleConf->m_strUinName.c_str());
        return -1;
    }
    /*******    
    if(GetUnIntValue(CManagerConf::Instance()->m_strUinName,m_uiUin))
    {
    ERROR_LOG("key[%s] not in req",CManagerConf::Instance()->m_strUinName.c_str());
    return -1;
    }
    *******/

    /*******
    if(m_uiUin<10000)
    {
    ERROR_LOG("uin[%u] illegal",m_uiUin);
    return -1;
    }	
    *******/

    if(GetUnIntValue(g_pCRuleConf->m_strAppIDName,m_uiAppID))
    {
        ERROR_LOG("key[%s] not in req", g_pCRuleConf->m_strAppIDName.c_str());
        return -1;
    }

    unsigned uiSource=0;
    if(GetUnIntValue(SOURCE,uiSource))
    {
        ERROR_LOG("key[%s] not in req",SOURCE);
        return -1;
    }    

    string strMod;
    if(GetStringValue(g_pCRuleConf->m_strModName,strMod))
    {
        ERROR_LOG("key[%s] not in req", g_pCRuleConf->m_strModName.c_str());
        return -1;
    }

    if(ParseModify(strMod))
    {
        return -1;
    }
#if 0
    map<string,vector<string> >::iterator it=m_mAttr2Op.find(STOCK_FEILD);
    if(m_mAttr2Op.end()!=it)//�ҵ���,˵���ǹ�Ʊ
    {
    //��Ҫ����ӳ��
    map< uint64_t, unsigned >::iterator it1=mapToUnsigned.find(ullUin);
    if(it1==mapToUnsigned.end())
    {
            ERROR_LOG("not find[%llu] in mapToUnsigned",ullUin);
            return -1;
    }
    m_uiUin=it1->second;
    DEBUG_LOG("stock uin[%u]ull[%llu]",m_uiUin,ullUin);
    }
    else
#endif
    {
    m_ullUin=ullUin;
    m_uiUin = (unsigned int)ullUin;
    //DEBUG_LOG("no stock uin[%u]ull[%llu]",m_uiUin,ullUin);
    }

    //��¼�����еĹؼ��ֶ�
    string strTime;
    if(GetStringValue(TIME_FEILD,strTime))
    {
        char szTemp[32];
        memset(szTemp,0,sizeof(szTemp));

        snprintf(szTemp,sizeof(szTemp),"%lu",time(NULL));
        strTime=szTemp;
    }    
    m_strReq.clear();
    m_strReq = g_pCRuleConf->m_strAppIDName+"="+m_mKey2Value[g_pCRuleConf->m_strAppIDName]+"&";
    m_strReq += g_pCRuleConf->m_strUinName+"="+m_mKey2Value[g_pCRuleConf->m_strUinName]+"&";
    m_strReq += g_pCRuleConf->m_strModName+"="+m_mKey2Value[g_pCRuleConf->m_strModName]+"&";
    m_strReq += string(SOURCE)+"="+m_mKey2Value[SOURCE]+"&";
    m_strReq += string(TIME_FEILD)+"="+strTime+"\n";

    if(3==uiSource)
    {
        m_Top.m_usDataType=E_TAXIS_DATA_TYPE_SYSTEM;
    }
    else
    {
        m_Top.m_usDataType=E_TAXIS_DATA_TYPE_USER;
    }

    //DEBUG_LOG("after map uin[%u]",m_uiUin);

    return 0;
}

string CDataProcess::GetReqMsg()
{
    return m_strReq;
}


int CDataProcess::ParseModify(string& strMValue)
{
    vector<string> vKey;
    CStrUtil::SplitStringUsing(strMValue, "|", &vKey);

    m_mAttr2Op.clear();

    vector<string>::iterator it;
    for(it=vKey.begin();it!=vKey.end();++it)
    {
        vector<string> vTemp;
        CStrUtil::SplitStringUsing(*it, "_", &vTemp);
        if(vTemp.size()!=4)
        {
            ERROR_LOG("value[%s]format error",(*it).c_str());
            continue;
        }

        m_mAttr2Op[vTemp[0]]=vTemp;
    }

    return 0;
}


int CDataProcess::SetTopMsg()
{
    //���bid�Ƿ���top���������ļ���
    map<unsigned int, set<string> >::iterator it1=g_cond_list.find(m_uiAppID);
    map<unsigned int, set<string> >::iterator it2=g_field_list.find(m_uiAppID);
    static unsigned int uiHandleTimes = 0;
    time_t tCountEndTime = 0;
    if(g_cond_list.end()==it1||g_field_list.end()==it2)
    {
        ERROR_LOG("bid[%u]is not in config file",m_uiAppID);
        return -1;
    }

    m_Top.m_usCondChange=0;
    m_Top.m_vConditionInfo.clear();
    m_Top.m_vFieldInfo.clear();

    set<string> sCond=g_cond_list[m_uiAppID];
    set<string> sField=g_field_list[m_uiAppID];

    set<string>::iterator sit;
    for(sit=sCond.begin();sit!=sCond.end();++sit)
    {
        DEBUG_LOG("g_cond_list field[%s]",(*sit).c_str());
    }

    for(sit=sField.begin();sit!=sField.end();++sit)
    {
        DEBUG_LOG("g_field_list field[%s]",(*sit).c_str());
    }

    map<string,vector<string> >::iterator it;

    Field_Info Info;
    vector<string> vTemp;
    unsigned short usOpt=E_TAXIS_MSG_OPT_MOD;
    for(it=m_mAttr2Op.begin();it!=m_mAttr2Op.end();++it)
    {
        vTemp=it->second;
        if(sCond.end()!=sCond.find(it->first))
        {
            Info.m_sField=vTemp[0];
            Info.m_uiValue=strtoul(vTemp[3].c_str(),NULL,0);
            m_Top.m_vConditionInfo.push_back(Info);
            DEBUG_LOG("m_vConditionInfo insert field[%s]",vTemp[0].c_str());

            //����¾�ֵ��һ����
            if(0!=vTemp[2].compare(vTemp[3]))
            {
                m_Top.m_usCondChange=1;
                DEBUG_LOG("m_vConditionInfo insert field[%s]oldvalue[%s]newvalue[%s]",
                vTemp[0].c_str(),vTemp[2].c_str(),vTemp[3].c_str());
            }
        }

        if(sField.end()!=sField.find(it->first))
        {
            Info.m_sField=vTemp[0];
            Info.m_uiValue=strtoul(vTemp[3].c_str(),NULL,0);
            m_Top.m_vFieldInfo.push_back(Info);
            DEBUG_LOG("m_vFieldInfo insert field[%s]",vTemp[0].c_str());
        }

        if(0==vTemp[1].compare("3"))
        {
            usOpt=E_TAXIS_MSG_OPT_DEL;
        }
    }

    m_Top.m_uiBid=m_uiAppID;
    m_Top.m_ullNumber=m_ullUin;
    m_Top.m_usOptType=usOpt;
    g_uiHandMsgFlag = 1;
    g_pCTopTaxisHandle->HandleMsg(m_Top);
    if(++uiHandleTimes > TAXIS_HANDLE_COUNT)
    {
        tCountEndTime = time(NULL);
        if(g_tCountBeginTime > tCountEndTime)
        {
            CRIT_LOG("Error!!!Record[%u]UseEndTime[%u]UseBeginTime[%u]", TAXIS_HANDLE_COUNT, tCountEndTime, 
                g_tCountBeginTime);
        }
        else
        {
            CRIT_LOG("Record[%u]UseTime[%u]", TAXIS_HANDLE_COUNT, tCountEndTime-g_tCountBeginTime);
        }
        uiHandleTimes = 0;
        g_tCountBeginTime = tCountEndTime;
    }
    g_uiHandMsgFlag = 0;

    return 0;
}


int CDataProcess::DeleteStore(vector<string>& vAttrOp,unsigned short& usBid)
{
    int iRet=0;
    map<unsigned short,CBitService*>::iterator it=m_mBid2Server.find(usBid);
    if(m_mBid2Server.end()==it)
    {
        ERROR_LOG("bid[%u] not in config tag server_list or not init",usBid);
        return -1;
    }

    CBitService* pBit=m_mBid2Server[usBid];

    vector<unsigned> vReq;
    vector<TResultNode> vRsp;
    vReq.push_back(m_uiUin);

    iRet=pBit->Delete(usBid,vReq,vRsp);
    if(iRet)
    {
        ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Delete fail ret[%d]errmsg[%s]",
        m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),iRet,pBit->GetErrMsg());
        return -1;
    }

    //����0,��Ҫ�����ж�key�ķ���ֵ
    if(vRsp.size()!=1)
    {
        ERROR_LOG("Delete ret rsp vector size[%u] not equal one uin[%u]attr[%s]opt[%s]bid[%u]value[%s]",
        vRsp.size(),m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());
        return -1;
    }

    TResultNode RspNode=vRsp[0];
    if(RspNode.m_sResultCode)
    {
        //ɾ��ʧ��
        ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Delete fail ret[%d]errmsg[%s]",
        m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),RspNode.m_sResultCode,pBit->GetErrMsg());
        return -1;
    }

    NOTI_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Delete succ",
    m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());	

    return 0;
}


int CDataProcess::InsertStore(vector<string>& vAttrOp,unsigned short& usBid)
{
    int iRet=0;
    map<unsigned short,CBitService*>::iterator it=m_mBid2Server.find(usBid);
    if(m_mBid2Server.end()==it)
    {
        ERROR_LOG("bid[%u] not in config tag server_list or not init",usBid);
        return -1;
    }

    CBitService* pBit=m_mBid2Server[usBid];

    vector<TKeyNode> vReq;
    vector<TResultNode> vRsp;
    TKeyNode Temp;
    Temp.m_uiKey=m_uiUin;
    Proto Data;
    Data.uiAttrValue=strtoul(vAttrOp[3].c_str(),NULL,10);
    Data.uiNewRank=0;
    Data.uiOldRank=0;
    Temp.m_strData.assign((const char*)&Data,sizeof(Data));
    vReq.push_back(Temp);

    iRet=pBit->Insert(usBid,vReq,vRsp);
    if(iRet)
    {
        ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Insert fail ret[%d]errmsg[%s]",
        m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),iRet,pBit->GetErrMsg());
        return -1;
    }

    //����0,��Ҫ�����ж�key�ķ���ֵ
    if(vRsp.size()!=1)
    {
        ERROR_LOG("Insert ret rsp vector size[%u] not equal one uin[%u]attr[%s]opt[%s]bid[%u]value[%s]",
        vRsp.size(),m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());
        return -1;
    }

    TResultNode RspNode=vRsp[0];
    if(RspNode.m_sResultCode)
    {
        //����ʧ��
        ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Insert fail ret[%d]errmsg[%s]",
        m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),RspNode.m_sResultCode,pBit->GetErrMsg());
        return -1;
    }

    NOTI_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] Insert succ",
    m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());	

    return 0;
}



int CDataProcess::UpdateStore(vector<string>& vAttrOp,unsigned short& usBid)
{
    int iRet=0;
    map<unsigned short,CBitService*>::iterator it=m_mBid2Server.find(usBid);
    if(m_mBid2Server.end()==it)
    {
        ERROR_LOG("bid[%u] not in config tag server_list or not init",usBid);
        return -1;
    }

    CBitService* pBit=m_mBid2Server[usBid];
    vector<TIntNode> vReq;
    vector<TRspNode> vRsp;
    TIntNode Temp;
    Temp.m_uiKey=m_uiUin;
    Temp.m_uiData=strtoul(vAttrOp[3].c_str(),NULL,10);
    Temp.m_uiMask=0xFFFFFFFF;
    vReq.push_back(Temp);

    iRet=pBit->SetInt(usBid,0,vReq,vRsp);
    if(iRet)
    {
        ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] SetInt fail ret[%d]errmsg[%s]",
        m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),iRet,pBit->GetErrMsg());
        return -1;
    }

    //����0,��Ҫ�����ж�ÿ��key�ķ���ֵ
    if(vRsp.size()!=1)
    {
        ERROR_LOG("SetInt ret rsp vector size[%u] not equal one uin[%u]attr[%s]opt[%s]bid[%u]value[%s]",
        vRsp.size(),m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());
        return -1;
    }

    TRspNode RspNode=vRsp[0];
    if(RspNode.m_sResultCode<0)
    {
        if(ERROR_DATA_NOT_EXIT==RspNode.m_sResultCode)
        {
            //����
            if(InsertStore(vAttrOp,usBid))
            {
                return -1;
            }
        }
        else
        {
            ERROR_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] SetInt fail ret[%d]errmsg[%s]",
            m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str(),RspNode.m_sResultCode,pBit->GetErrMsg());
            return -1;
        }
    }

    NOTI_LOG("uin[%u]attr[%s]opt[%s]bid[%u]value[%s] UpdateStore succ",
    m_uiUin,vAttrOp[0].c_str(),vAttrOp[1].c_str(),usBid,vAttrOp[3].c_str());

    return 0;
}

int CDataProcess::SetStore(vector<string>& vAttrOp,unsigned short& usBid)
{
    int iOpt=atoi(vAttrOp[1].c_str());
    if(1==iOpt||2==iOpt)
    {
        if(UpdateStore(vAttrOp,usBid))
        {
            ERROR_LOG("UpdateStore error");
            return -1;
        }
    }
    else if(3==iOpt)
    {
        if(DeleteStore(vAttrOp,usBid))
        {
            ERROR_LOG("DeleteStore error");
            return -1;
        }
    }
    else
    {
        ERROR_LOG("uin[%u]attr[%s]opt[%d]bid[%u]value[%s] opt illegal",
        m_uiUin,vAttrOp[0].c_str(),iOpt,usBid,vAttrOp[3].c_str());
        return -1;
    }

    return 0;
}


int CDataProcess::Process()
{
    //��appid�ֶ�,Ȼ���ж��Ƿ��������ļ��������˴�appid
    map<unsigned,map<string,unsigned short> >::iterator it = g_pCRuleConf->m_mApp2Attr.find(m_uiAppID);
    if(g_pCRuleConf->m_mApp2Attr.end() == it)
    {
        ERROR_LOG("req appid[%u] not in config file",m_uiAppID);
        return -1;
    }

    //��֤�����ֶ��Ƿ�����
    map<string,unsigned short> mTemp=it->second;
    map<string,unsigned short>::iterator itAttr2Bid;
    map<string,vector<string> >::iterator itAttr2Op;
    for(itAttr2Op=m_mAttr2Op.begin();itAttr2Op!=m_mAttr2Op.end();++itAttr2Op)
    {
        itAttr2Bid=mTemp.find(itAttr2Op->first);
        if(itAttr2Bid!=mTemp.end())
        {
            //������˵���ϱ��������������õ������ֶ�,���º�˶�Ӧ��bitservice
            if(SetStore(itAttr2Op->second,itAttr2Bid->second))
            {
                ERROR_LOG("SetStore error bid[%u]",itAttr2Bid->second);
            }
        }
        else
        {
            DEBUG_LOG("req appid[%u]attr[%s] not in config file",m_uiAppID,itAttr2Op->first.c_str());
        }
    }

    return 0;
}
