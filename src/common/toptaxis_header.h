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
*      Description: TOP排序系统结构通讯协议定义
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
/*top文件的样式 ../taxisdata/taxis_2001_1_20*/
/* ../taxisdata/taxis_ruleid_condnum_cond1_cond2...*/
#define TAXIS_MIN_FILED_TOPFILE                         3

#define TAXIS_MAX_CMD_LEN                               256

/*A_0000010000_1234567890\n*/
#define TAXIS_LENGTH_HEAD_FILED_TOPFILE                 25

/*10字节空白填充*/
#define TAXIS_TEN_NULL_CHAR                             "          "
#define TAXIS_TEN_SIZE_CHAR                             10

#define TAXIS_MAX_UNSIGN_INT                            12

#define TAXIS_LAST_DATA_DIR                             "../taxisdata"
#define TAXIS_LAST_DATA_PREFIX                          "taxis_"
/*保存类似股票涨幅跌幅的原始值(前一天，前一周)*/
#define TAXIS_HISTORY_DATA_DIR                          "../historydata"
#define TAXIS_HISTORY_DATA_PREFIX                       "history_"
#define TAXIS_HISTORY_TMP_DATA_SUFFIX                   ".tmp"

#define TAXIS_COLDBAK_DATA_DIR                          "../colddata"
#define TAXIS_COLDBAK_DATA_BAK_DIR                      "../colddata/bak"
#define TAXIS_COLDBAK_OK_FLAG                           "OK"
/*按天排序的只记录3个数据*/
/*Uin_curdata_history0\n*/
#define TAXIS_HISTORY_DATA_NUM_DAY                      3
/*按天排序的只记录7个数据*/
/*Uin_curdata_history0_history1_history2_history3_history4_history5_history6\n*/
#define TAXIS_HISTORY_DATA_NUM_WEEK                     9
#define TAXIS_HISTORY_DATA_MAX_LEN                      256
#define TAXIS_CHANGE_DATA_RATE                          100000    /*增幅涨幅按照百分比排名，精确到小数点三位*/
#define WEEK_DAYS                                       7

#define MAX_NAME_LEN                                    64        //最大热备流水文件名长度
#define ERR_FD_NO                                       -1        //初始文件句柄号

#define TAXIS_DEL_SUCC                                  '0'
#define TAXIS_DEL_FAIL                                  '1'
#define TAXIS_DEL_RSP_LEN                                1
#define TAXIS_MIN_UIN                                    10000
#define MAXTIMES_TAXIS_RECV_TRY                          2000
enum TAXIS_LISTEN_PORT
{
    E_PORT_TAXIS_WRITE                       = 32101,   //toptaxis接收消息的端口
};

enum TAXIS_MSG_OPT
{
    E_TAXIS_MSG_OPT_ADD                       = 0,   //toptaxis消息新增数据
    E_TAXIS_MSG_OPT_MOD                       = 1,   //toptaxis消息修改数据
    E_TAXIS_MSG_OPT_DEL                       = 2,   //toptaxis消息删除数据
};

enum TAXIS_DATA_TYPE
{
    E_TAXIS_DATA_TYPE_USER                       = 0,   //与用户相关联的数据: 比如用户的金币、资产等
    E_TAXIS_DATA_TYPE_SYSTEM                     = 1,   //与系统想关联的数据: 比如股市里面的股票等
};

/*排序所采集数据的周期*/
enum TAXIS_TIME_TYPE
{
    E_REAL_DATA                              = 0,  // 实时数据进行排序
    E_DAY_DATA                               = 1, // 一天的变化数据进行排序
    E_WEEK_DATA                              = 2, // 一周的变化数据进行排序
};

/*排序所采集数据的周期*/
enum TAXIS_WATERLOG_TYPE
{
    E_TAXIS_COLD_WATERLOG                    = 0,  // 冷备流水日志
    E_TAXIS_REAL_WATERLOG                    = 1,  //实时流水日志
};

/*排序的规则*/
enum TAXIS_TOP_TYPE
{
    E_TOP_UP                                 = 0,  // 升序排列(最大值排在前面)
    E_TOP_DOWN                               = 1, // 降序排列(最小值排在前面)
};

enum STORE_TO_NOTIFY_COMMAND 
{
    E_SERVICE_NOTI                           = 8001,  //排序变更,通知notify同步
};

enum TAXIS_RULE_ENABLE_FLAG 
{
    E_RULE_DISABLE                           = 0,  //规则无效
    E_RULE_ENABLE                            = 1,  //规则有效
};

enum FATHER_COMMAND 
{
    FATHER_TRANSFER_TO_STORE                 = 1001,  //外部所有系统到toptaxis的命令汇总
    FATHER_TRANSFER_TO_CONTROL               = 1002,  //保留字段，暂时不用

    FATHER_CONTROL_TO_TRANSFER               = 1003,  //保留字段，暂时不用
    FATHER_CONTROL_TO_STORE                  = 1004,  //保留字段，暂时不用

    FATHER_STORE_TO_CONTROL                  = 1005,  //保留字段，暂时不用
    FATHER_CGI_TO_CONTROL                    = 1006,  //保留字段，暂时不用

    E_FATHER_STORE_TO_NOTIFY                 = 1007,  //toptaxis到notify的命令汇总
};

enum TRASNFER_TO_STORE_COMMAND 
{
    SERVICE_INSERT                           = 2001,  //保留字段，暂时不用
    SERVICE_DELETE                           = 2002,  //删除排序数据
    SERVICE_RELOAD                           = 2003,  //通知Reload配置文件
    SERVICE_QUERY                            = 2004,  //查询排序数据

    SERVICE_REPLACE                          = 2005,  //数据覆盖（支持批量），主要用于日志同步
    SERVICE_RESYNC                           = 2006,  //保留字段，暂时不用

    SERVICE_SETBIT                           = 2008,  //保留字段，暂时不用
    SERVICE_SET_BYTE_BIT                     = 2009,  //保留字段，暂时不用
    SERVICE_SET_SHORT_BIT                    = 2010,  //保留字段，暂时不用
    SERVICE_SET_INT_BIT                      = 2011,  //保留字段，暂时不用
    SERVICE_SET_LONGLONG_BIT                 = 2012,  //保留字段，暂时不用
};

/*类似股票等需要保存过去的历史数据*/
typedef struct _All_History_Value
{
    vector <int> m_vuiInitData;     //过去的历史数据
    int m_uiCurData;                //当前的实时值
}All_History_Value;

typedef map<unsigned long long, All_History_Value> MAP_HISTORY_VALUE;

/*白名单号码列表*/
typedef vector<unsigned long long> V_WHITE_LIST;

typedef struct _Sort_Pos
{
    int m_i_firstpos;                          //排序之前的位置
    int m_i_lastpos;                           //排序之后的位置
}Sort_Pos_Info;

typedef struct _Field_Info
{
    string m_sField;                            //属性字段编号，系统会通过读取规则文件为每个属性编号
    unsigned int m_uiValue;                     //属性字段的值
}Field_Info;

typedef struct _Key_Value
{
    unsigned long long m_ullKey;                //属性的Key, 如果为用户属性，则为QQ号码，如果为股票等全局属性，则为股票编码
    int m_iValue;                               //key所对应的值
    int m_iChangeValue;                         //key所对应的涨幅跌幅值
}Key_Value;

typedef struct _Toptaxis_Msg
{
    unsigned long long m_ullNumber;              //用户号码
    unsigned int m_uiBid;                        //业务编码
    unsigned short m_usCondChange;               //条件值发生变化，比如城市发生变化
    unsigned short m_usOptType;                  //操作标识(删除和新增)
    unsigned short m_usDataType;                 //消息所承载的数据类型
    vector <Field_Info> m_vFieldInfo;            //属性字段列表
    vector <Field_Info> m_vConditionInfo;        //条件字段列表
}Toptaxis_Msg;

typedef struct _Top_Info
{
    vector <unsigned int> m_vUiConditionValue;   //多条件(|分割)属性所对应的多份值	
    vector <Key_Value> m_vKeyValueList;          //规则ID对应着的排序【QQ号码+值】，Condition_Info的 m_usFieldType为0有效
    FILE* m_lasttopfd;                           //记录最新的top排序的共享内存的key值
    unsigned int m_uiShmId;                      //共享内存ID 
    void* m_pDataAddr;                           //共享内存的首地址
}Top_Info;

typedef struct _Rule_Info
{
    unsigned int m_uiRuleId;                 //规则ID
    unsigned int m_uiTopNum;                 //排序的数目
    unsigned int m_uiTopDisplayNum;          //排序后所能查询到的数目
    char m_cUpdateFlag;                      //记录重启后是否需要更新数据。
    unsigned short m_usTaxisType;            // 0 -----排最大值 1-----排最小值	
    string m_sFieldId;                       //属性的ID
    vector <Top_Info> m_vTopList;            //规则ID对应着的排序
    MAP_HISTORY_VALUE m_mapHostoryList;
    FILE* m_posfd;                           //文件句柄，该文件记录该规则所有记录排序列表的文件名
    FILE* m_historyfd;                       //文件句柄，该文件记录保存类似股票涨幅跌幅的原始值(前一天，前一周)
    unsigned char m_cEnableFlag;             //规则是否生效
    V_WHITE_LIST m_vWhiteList;               //白名单列表
}Rule_Info;


typedef struct _Condition_Info
{
    vector <string> m_vCondition;            //条件所对应的属性(可能包含多个条件子项)
    unsigned short m_usCondId;               //唯一标识配置文件中的一个配置条件
    vector <Rule_Info> m_vRuleList;          //业务所对应的规则列表
    unsigned short m_usTaxisTime;            //0----排当前值1-----排当天的增减值2-----排当周的值增减值
    unsigned int m_uiSerId;                  //业务id, toptaxis只需要透传给业务客户端
    V_WHITE_LIST m_vWhiteList;               //白名单列表
}Condition_Info;

/*业务的所有条件信息*/
typedef struct _Bid_Condition
{
    unsigned int m_uiBid;                            //业务ID
    vector <Condition_Info> m_vConditionList;        //业务ID里面包含的条件信息		
    V_WHITE_LIST m_vWhiteList;                       //白名单列表
}Bid_Condition;

/*记录用户在哪个条件规则里面存在排序*/
typedef struct _Toptaxis_Pos
{
    unsigned int m_uiBidIndex;            //业务ID索引
    unsigned int m_uiConditionListIndex;  //业务ID里面包含的条件信息索引
    unsigned int m_uiRuleListIndex;       //业务所对应的规则列表索引
    unsigned int m_uiTopListIndex;        //规则ID对应着的排序索引		
}Toptaxis_Pos;

typedef struct _TopTaxisShmData
{
    Toptaxis_Pos m_stTaxisPos;
    unsigned short m_usCondNum;
    unsigned m_auiCond[TAXIS_MAX_COND_NUM];

    unsigned int m_uiANum;//A块数据记录的排序节点个数
    unsigned int m_uiBNum;//B块数据记录的排序节点个数
    unsigned char m_ucABFlag;//上次数据在ab记录的哪一块
    Key_Value m_tAData[TAXIS_MAX_TOP_NUM];//A块数据
    Key_Value m_tBData[TAXIS_MAX_TOP_NUM];//B块数据
}TopTaxisShmData;


typedef vector<Toptaxis_Pos> V_Toptaxis_Pos;
typedef map<unsigned long long, V_Toptaxis_Pos> MAP_TAXIS_POS;

/*所有的规则链表*/
typedef map<unsigned int, Toptaxis_Pos> MAP_TAXIS_RULE_LIST;

/*规则配置文件与bid的映射表*/
typedef map<string, unsigned int> MAP_TAXIS_RULECONF_BID;

/*流水同步的消息结构*/
typedef struct _Sync_Toptaxis
{
    unsigned int m_uiLength;                      //整个结构的长度
    Toptaxis_Pos m_taxispos;                     //在条件规则中的位置
    unsigned int m_uiConditionValueNum;   //多条件(|分割)属性所对应的多份值	
    unsigned int m_uiTopListNum;              //排序列表中的元素个数
    char * pData[0];                                  //前面存储条件属性的值，后面存储排序的节点
}Sync_Toptaxis;

typedef struct _String_Info
{
    unsigned int m_uiBid;            //业务ID
    map <string, unsigned char> m_vString;        //业务所对应的条件属性列表
}String_Info;

/*前端查询top排序结果的请求体*/
typedef struct _Query_Taxis_Req
{
    unsigned short m_usBID; 
    unsigned short m_usCondId;
    unsigned short m_usValueNum;
    unsigned int m_uiRuleId;
    unsigned int  m_uiCondValue[0];//查询的条件值列表
}Query_Taxis_Req;

/*前端查询top排序结果的回应内容*/
typedef struct Taxis_Query_Body_Rsp
{
    unsigned long long  m_ullKey;
    int m_iValue;
    int m_iChange;
}Taxis_Query_Body_Rsp;
/*前端查询top排序结果的请求体*/
typedef struct _Query_Taxis_Rsp
{
    short m_sResultCode;                 //整个命令的操作结果
    unsigned short  m_usKeyNum;   
    unsigned int m_uiSerId;
    Taxis_Query_Body_Rsp m_szMsgBody[0];
}Query_Taxis_Rsp;

/*排序变更,通知notify请求体*/
typedef struct _NotiTaxisReq
{
    unsigned m_uiKey;
}NotiTaxisReq;

/*排序变更,notify响应体*/
typedef struct _NotiTaxisRsp
{
    int  m_iResultCode;
}NotiTaxisRsp;

typedef struct _Bitmap_Header
{
    unsigned char m_cVersion;                    //固定为0xFF
    unsigned short m_usMsgLen;                     //msg length	
    unsigned short m_usFirstLevelCmd;            //first level cmd length	
    unsigned short m_usSecondLevelCmd;           //second level cmd length	
    unsigned int m_uiReserved;                              //reserved	
    char m_szMsgData[0];
}Bitmap_Header;

typedef struct _Replace_Base_Data
{     
    unsigned short m_usDataLen;        //单条同步消息的长度
    char m_szDataBuf[0];                   //单条同步消息的内容
}Replace_Base_Data;

typedef struct _Replace_Data_Req
{     
    char m_szHardSync;                         //是否采用强制同步，如果为1，STORE不判断偏移量
    unsigned int  m_uiFileName;              //同步消息所在文件的名称
    unsigned int m_uiLineNum;                //第一条同步消息在文件中的行号
    unsigned long  m_ulOffSet;                //第一条同步消息在文件中的偏移量
    unsigned short m_usRecordNum;      //同步消息的个数
    Replace_Base_Data m_struData[0];  //同步消息的内容
}Replace_Data_Req;

typedef struct _Replace_Data_Rsp
{
    short m_sResultCode;                  //返回码，只有在偏移量错误下，后面三个字段才有意义
    unsigned int  m_uiFileName;          //共享内存中记录的同步文件名称
    unsigned int m_uiLineNum;            //共享内存中记录的同步文件当前的行号
    unsigned long  m_ulOffSet;            //共享内存中记录的同步文件当前的行尾对应的偏移量
}Replace_Data_Rsp;

#pragma pack()
#endif
