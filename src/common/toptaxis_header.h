/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_header.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP����ϵͳ�ṹͨѶЭ�鶨��
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_HEADER_H_
#define _TOPTAXIS_HEADER_H_
#include <vector>
#include <string>
#include <map>
#include <set>
#include "error_code.h"
using namespace std;
#pragma pack(1)

/***************************************************************************/
/**************************Important data type ********************************/
/***************************************************************************/
#define  BITSERVICE_VERSION                             "OpenCloud_V1.4.3_ctaxis1.0.0_Build0001"

#define LENGTH_OF_PREHEAD_TAXIS                         3
#define TAXIS_MAX_TOP_NUM                               3000
#define TAXIS_MAX_DISPLAY_TOP_NUM                       300
#define TAXIS_MAX_COND_NUM                              20
#define TAXIS_MAX_WATER_LOG                             32000
#define TAXIS_MAX_WATER_COLD_LOG                        240000
#define TAXIS_MAX_MSG_LOG                               32600
#define TAXIS_SHM_SIZE                                  411200
#define TAXIS_MAX_WATER_NULL_LOG                        20560
#define TAXIS_MAX_HEAD_WATER_LOG                        320
#define TAXIS_MAX_RECORD_LEN                            40
#define TAXIS_MINNUM_WATER_LOG                          6
#define TAXIS_NUM_KEY_VALUE                             3
/*top�ļ�����ʽ ../taxisdata/taxis_2001_1_20*/
/* ../taxisdata/taxis_ruleid_condnum_cond1_cond2...*/
#define TAXIS_MIN_FILED_TOPFILE                         3

#define TAXIS_MAX_CMD_LEN                               256

/*A_0000010000_1234567890\n*/
#define TAXIS_LENGTH_HEAD_FILED_TOPFILE                 25

/*10�ֽڿհ����*/
#define TAXIS_TEN_NULL_CHAR                             "          "
#define TAXIS_TEN_SIZE_CHAR                             10

#define TAXIS_MAX_UNSIGN_INT                            12

#define TAXIS_LAST_DATA_DIR                             "../taxisdata"
#define TAXIS_LAST_DATA_PREFIX                          "taxis_"
/*�������ƹ�Ʊ�Ƿ�������ԭʼֵ(ǰһ�죬ǰһ��)*/
#define TAXIS_HISTORY_DATA_DIR                          "../historydata"
#define TAXIS_HISTORY_DATA_PREFIX                       "history_"
#define TAXIS_HISTORY_TMP_DATA_SUFFIX                   ".tmp"

#define TAXIS_COLDBAK_DATA_DIR                          "../colddata"
#define TAXIS_COLDBAK_DATA_BAK_DIR                      "../colddata/bak"
#define TAXIS_COLDBAK_OK_FLAG                           "OK"
/*���������ֻ��¼3������*/
/*Uin_curdata_history0\n*/
#define TAXIS_HISTORY_DATA_NUM_DAY                      3
/*���������ֻ��¼7������*/
/*Uin_curdata_history0_history1_history2_history3_history4_history5_history6\n*/
#define TAXIS_HISTORY_DATA_NUM_WEEK                     9
#define TAXIS_HISTORY_DATA_MAX_LEN                      256
#define TAXIS_CHANGE_DATA_RATE                          100000    /*�����Ƿ����հٷֱ���������ȷ��С������λ*/
#define WEEK_DAYS                                       7

#define MAX_NAME_LEN                                    64        //����ȱ���ˮ�ļ�������
#define ERR_FD_NO                                       -1        //��ʼ�ļ������

#define TAXIS_DEL_SUCC                                  '0'
#define TAXIS_DEL_FAIL                                  '1'
#define TAXIS_DEL_RSP_LEN                                1
#define TAXIS_MIN_UIN                                    10000
#define MAXTIMES_TAXIS_RECV_TRY                          2000
enum TAXIS_LISTEN_PORT
{
    E_PORT_TAXIS_WRITE                       = 32101,   //toptaxis������Ϣ�Ķ˿�
};

enum TAXIS_MSG_OPT
{
    E_TAXIS_MSG_OPT_ADD                       = 0,   //toptaxis��Ϣ��������
    E_TAXIS_MSG_OPT_MOD                       = 1,   //toptaxis��Ϣ�޸�����
    E_TAXIS_MSG_OPT_DEL                       = 2,   //toptaxis��Ϣɾ������
};

enum TAXIS_DATA_TYPE
{
    E_TAXIS_DATA_TYPE_USER                       = 0,   //���û������������: �����û��Ľ�ҡ��ʲ���
    E_TAXIS_DATA_TYPE_SYSTEM                     = 1,   //��ϵͳ�����������: �����������Ĺ�Ʊ��
};

/*�������ɼ����ݵ�����*/
enum TAXIS_TIME_TYPE
{
    E_REAL_DATA                              = 0,  // ʵʱ���ݽ�������
    E_DAY_DATA                               = 1, // һ��ı仯���ݽ�������
    E_WEEK_DATA                              = 2, // һ�ܵı仯���ݽ�������
};

/*�������ɼ����ݵ�����*/
enum TAXIS_WATERLOG_TYPE
{
    E_TAXIS_COLD_WATERLOG                    = 0,  // �䱸��ˮ��־
    E_TAXIS_REAL_WATERLOG                    = 1,  //ʵʱ��ˮ��־
};

/*����Ĺ���*/
enum TAXIS_TOP_TYPE
{
    E_TOP_UP                                 = 0,  // ��������(���ֵ����ǰ��)
    E_TOP_DOWN                               = 1, // ��������(��Сֵ����ǰ��)
};

enum STORE_TO_NOTIFY_COMMAND 
{
    E_SERVICE_NOTI                           = 8001,  //������,֪ͨnotifyͬ��
};

enum TAXIS_RULE_ENABLE_FLAG 
{
    E_RULE_DISABLE                           = 0,  //������Ч
    E_RULE_ENABLE                            = 1,  //������Ч
};

enum FATHER_COMMAND 
{
    FATHER_TRANSFER_TO_STORE                 = 1001,  //�ⲿ����ϵͳ��toptaxis���������
    FATHER_TRANSFER_TO_CONTROL               = 1002,  //�����ֶΣ���ʱ����

    FATHER_CONTROL_TO_TRANSFER               = 1003,  //�����ֶΣ���ʱ����
    FATHER_CONTROL_TO_STORE                  = 1004,  //�����ֶΣ���ʱ����

    FATHER_STORE_TO_CONTROL                  = 1005,  //�����ֶΣ���ʱ����
    FATHER_CGI_TO_CONTROL                    = 1006,  //�����ֶΣ���ʱ����

    E_FATHER_STORE_TO_NOTIFY                 = 1007,  //toptaxis��notify���������
};

enum TRASNFER_TO_STORE_COMMAND 
{
    SERVICE_INSERT                           = 2001,  //�����ֶΣ���ʱ����
    SERVICE_DELETE                           = 2002,  //ɾ����������
    SERVICE_RELOAD                           = 2003,  //֪ͨReload�����ļ�
    SERVICE_QUERY                            = 2004,  //��ѯ��������

    SERVICE_REPLACE                          = 2005,  //���ݸ��ǣ�֧������������Ҫ������־ͬ��
    SERVICE_RESYNC                           = 2006,  //�����ֶΣ���ʱ����

    SERVICE_SETBIT                           = 2008,  //�����ֶΣ���ʱ����
    SERVICE_SET_BYTE_BIT                     = 2009,  //�����ֶΣ���ʱ����
    SERVICE_SET_SHORT_BIT                    = 2010,  //�����ֶΣ���ʱ����
    SERVICE_SET_INT_BIT                      = 2011,  //�����ֶΣ���ʱ����
    SERVICE_SET_LONGLONG_BIT                 = 2012,  //�����ֶΣ���ʱ����
};

/*���ƹ�Ʊ����Ҫ�����ȥ����ʷ����*/
typedef struct _All_History_Value
{
    vector <int> m_vuiInitData;     //��ȥ����ʷ����
    int m_uiCurData;                //��ǰ��ʵʱֵ
}All_History_Value;

typedef map<unsigned long long, All_History_Value> MAP_HISTORY_VALUE;

/*�����������б�*/
typedef vector<unsigned long long> V_WHITE_LIST;

typedef struct _Sort_Pos
{
    int m_i_firstpos;                          //����֮ǰ��λ��
    int m_i_lastpos;                           //����֮���λ��
}Sort_Pos_Info;

typedef struct _Field_Info
{
    string m_sField;                            //�����ֶα�ţ�ϵͳ��ͨ����ȡ�����ļ�Ϊÿ�����Ա��
    unsigned int m_uiValue;                     //�����ֶε�ֵ
}Field_Info;

typedef struct _Key_Value
{
    unsigned long long m_ullKey;                //���Ե�Key, ���Ϊ�û����ԣ���ΪQQ���룬���Ϊ��Ʊ��ȫ�����ԣ���Ϊ��Ʊ����
    int m_iValue;                               //key����Ӧ��ֵ
    int m_iChangeValue;                         //key����Ӧ���Ƿ�����ֵ
}Key_Value;

typedef struct _Toptaxis_Msg
{
    unsigned long long m_ullNumber;              //�û�����
    unsigned int m_uiBid;                        //ҵ�����
    unsigned short m_usCondChange;               //����ֵ�����仯��������з����仯
    unsigned short m_usOptType;                  //������ʶ(ɾ��������)
    unsigned short m_usDataType;                 //��Ϣ�����ص���������
    vector <Field_Info> m_vFieldInfo;            //�����ֶ��б�
    vector <Field_Info> m_vConditionInfo;        //�����ֶ��б�
}Toptaxis_Msg;

typedef struct _Top_Info
{
    vector <unsigned int> m_vUiConditionValue;   //������(|�ָ�)��������Ӧ�Ķ��ֵ	
    vector <Key_Value> m_vKeyValueList;          //����ID��Ӧ�ŵ�����QQ����+ֵ����Condition_Info�� m_usFieldTypeΪ0��Ч
    FILE* m_lasttopfd;                           //��¼���µ�top����Ĺ����ڴ��keyֵ
    unsigned int m_uiShmId;                      //�����ڴ�ID 
    void* m_pDataAddr;                           //�����ڴ���׵�ַ
}Top_Info;

typedef struct _Rule_Info
{
    unsigned int m_uiRuleId;                 //����ID
    unsigned int m_uiTopNum;                 //�������Ŀ
    unsigned int m_uiTopDisplayNum;          //��������ܲ�ѯ������Ŀ
    char m_cUpdateFlag;                      //��¼�������Ƿ���Ҫ�������ݡ�
    unsigned short m_usTaxisType;            // 0 -----�����ֵ 1-----����Сֵ	
    string m_sFieldId;                       //���Ե�ID
    vector <Top_Info> m_vTopList;            //����ID��Ӧ�ŵ�����
    MAP_HISTORY_VALUE m_mapHostoryList;
    FILE* m_posfd;                           //�ļ���������ļ���¼�ù������м�¼�����б���ļ���
    FILE* m_historyfd;                       //�ļ���������ļ���¼�������ƹ�Ʊ�Ƿ�������ԭʼֵ(ǰһ�죬ǰһ��)
    unsigned char m_cEnableFlag;             //�����Ƿ���Ч
    V_WHITE_LIST m_vWhiteList;               //�������б�
}Rule_Info;


typedef struct _Condition_Info
{
    vector <string> m_vCondition;            //��������Ӧ������(���ܰ��������������)
    unsigned short m_usCondId;               //Ψһ��ʶ�����ļ��е�һ����������
    vector <Rule_Info> m_vRuleList;          //ҵ������Ӧ�Ĺ����б�
    unsigned short m_usTaxisTime;            //0----�ŵ�ǰֵ1-----�ŵ��������ֵ2-----�ŵ��ܵ�ֵ����ֵ
    unsigned int m_uiSerId;                  //ҵ��id, toptaxisֻ��Ҫ͸����ҵ��ͻ���
    V_WHITE_LIST m_vWhiteList;               //�������б�
}Condition_Info;

/*ҵ�������������Ϣ*/
typedef struct _Bid_Condition
{
    unsigned int m_uiBid;                            //ҵ��ID
    vector <Condition_Info> m_vConditionList;        //ҵ��ID���������������Ϣ		
    V_WHITE_LIST m_vWhiteList;                       //�������б�
}Bid_Condition;

/*��¼�û����ĸ��������������������*/
typedef struct _Toptaxis_Pos
{
    unsigned int m_uiBidIndex;            //ҵ��ID����
    unsigned int m_uiConditionListIndex;  //ҵ��ID���������������Ϣ����
    unsigned int m_uiRuleListIndex;       //ҵ������Ӧ�Ĺ����б�����
    unsigned int m_uiTopListIndex;        //����ID��Ӧ�ŵ���������		
}Toptaxis_Pos;

typedef struct _TopTaxisShmData
{
    Toptaxis_Pos m_stTaxisPos;
    unsigned short m_usCondNum;
    unsigned m_auiCond[TAXIS_MAX_COND_NUM];

    unsigned int m_uiANum;//A�����ݼ�¼������ڵ����
    unsigned int m_uiBNum;//B�����ݼ�¼������ڵ����
    unsigned char m_ucABFlag;//�ϴ�������ab��¼����һ��
    Key_Value m_tAData[TAXIS_MAX_TOP_NUM];//A������
    Key_Value m_tBData[TAXIS_MAX_TOP_NUM];//B������
}TopTaxisShmData;


typedef vector<Toptaxis_Pos> V_Toptaxis_Pos;
typedef map<unsigned long long, V_Toptaxis_Pos> MAP_TAXIS_POS;

/*���еĹ�������*/
typedef map<unsigned int, Toptaxis_Pos> MAP_TAXIS_RULE_LIST;

/*���������ļ���bid��ӳ���*/
typedef map<string, unsigned int> MAP_TAXIS_RULECONF_BID;

/*��ˮͬ������Ϣ�ṹ*/
typedef struct _Sync_Toptaxis
{
    unsigned int m_uiLength;                      //�����ṹ�ĳ���
    Toptaxis_Pos m_taxispos;                     //�����������е�λ��
    unsigned int m_uiConditionValueNum;   //������(|�ָ�)��������Ӧ�Ķ��ֵ	
    unsigned int m_uiTopListNum;              //�����б��е�Ԫ�ظ���
    char * pData[0];                                  //ǰ��洢�������Ե�ֵ������洢����Ľڵ�
}Sync_Toptaxis;

typedef struct _String_Info
{
    unsigned int m_uiBid;            //ҵ��ID
    map <string, unsigned char> m_vString;        //ҵ������Ӧ�����������б�
}String_Info;

/*ǰ�˲�ѯtop��������������*/
typedef struct _Query_Taxis_Req
{
    unsigned short m_usBID; 
    unsigned short m_usCondId;
    unsigned short m_usValueNum;
    unsigned int m_uiRuleId;
    unsigned int  m_uiCondValue[0];//��ѯ������ֵ�б�
}Query_Taxis_Req;

/*ǰ�˲�ѯtop�������Ļ�Ӧ����*/
typedef struct Taxis_Query_Body_Rsp
{
    unsigned long long  m_ullKey;
    int m_iValue;
    int m_iChange;
}Taxis_Query_Body_Rsp;
/*ǰ�˲�ѯtop��������������*/
typedef struct _Query_Taxis_Rsp
{
    short m_sResultCode;                 //��������Ĳ������
    unsigned short  m_usKeyNum;   
    unsigned int m_uiSerId;
    Taxis_Query_Body_Rsp m_szMsgBody[0];
}Query_Taxis_Rsp;

/*������,֪ͨnotify������*/
typedef struct _NotiTaxisReq
{
    unsigned m_uiKey;
}NotiTaxisReq;

/*������,notify��Ӧ��*/
typedef struct _NotiTaxisRsp
{
    int  m_iResultCode;
}NotiTaxisRsp;

typedef struct _Bitmap_Header
{
    unsigned char m_cVersion;                    //�̶�Ϊ0xFF
    unsigned short m_usMsgLen;                     //msg length	
    unsigned short m_usFirstLevelCmd;            //first level cmd length	
    unsigned short m_usSecondLevelCmd;           //second level cmd length	
    unsigned int m_uiReserved;                              //reserved	
    char m_szMsgData[0];
}Bitmap_Header;

typedef struct _Replace_Base_Data
{     
    unsigned short m_usDataLen;        //����ͬ����Ϣ�ĳ���
    char m_szDataBuf[0];                   //����ͬ����Ϣ������
}Replace_Base_Data;

typedef struct _Replace_Data_Req
{     
    char m_szHardSync;                         //�Ƿ����ǿ��ͬ�������Ϊ1��STORE���ж�ƫ����
    unsigned int  m_uiFileName;              //ͬ����Ϣ�����ļ�������
    unsigned int m_uiLineNum;                //��һ��ͬ����Ϣ���ļ��е��к�
    unsigned long  m_ulOffSet;                //��һ��ͬ����Ϣ���ļ��е�ƫ����
    unsigned short m_usRecordNum;      //ͬ����Ϣ�ĸ���
    Replace_Base_Data m_struData[0];  //ͬ����Ϣ������
}Replace_Data_Req;

typedef struct _Replace_Data_Rsp
{
    short m_sResultCode;                  //�����룬ֻ����ƫ���������£����������ֶβ�������
    unsigned int  m_uiFileName;          //�����ڴ��м�¼��ͬ���ļ�����
    unsigned int m_uiLineNum;            //�����ڴ��м�¼��ͬ���ļ���ǰ���к�
    unsigned long  m_ulOffSet;            //�����ڴ��м�¼��ͬ���ļ���ǰ����β��Ӧ��ƫ����
}Replace_Data_Rsp;

#pragma pack()
#endif
