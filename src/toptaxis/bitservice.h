/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*
 ****************************************************************************
 * file: bitservice.h 
 * desc: api definition for bitservice
 * copyright: Tencent
 * author: 
 * create: 2011/03/17
 ****************************************************************************
 */

#ifndef __BITSERVICE_API__
#define __BITSERVICE_API__

#include <string>
#include <vector>
#include <map>
using namespace std;

namespace BitService
{

#define MAX_NAMESER_LEN 		256		//名字长度
#define MAX_BATCH_KEYS_NUM	100		//批量处理最大数目
#define MAX_MSG_LEN 		4096	//包体最大长度
#define MAX_BITSET_LEN 		32		//设置位最大长度
#define MAX_ERR_MSG_LEN 	256		//错误信息最大长度
#define MAX_IP_ADDR_LEN 	16		//IP地址长度

#pragma pack(1)
typedef map<string, void*> MapIpSocket;

// 全量数据操作响应结点
typedef struct
{
	short m_sResultCode;  //0表示操作成功，其他表示store服务器返回的错误码
	unsigned int m_uiKey; //键
	string m_strData;	  //数据内容
}TResultNode;

// 全量数据操作的结点
typedef struct
{
	unsigned int m_uiKey; //键
	string m_strData;	  //数据内容
}TKeyNode;

// 设置Bit结点
typedef struct
{
	unsigned int  m_uiKey;	       //操作的KEY值
	unsigned int m_uiData;
}TBitNode;

// 设置Byte结点
typedef struct
{
	unsigned int  m_uiKey;	       //操作的KEY值
	char m_szData;                 //操作的数据
	char m_szMask;                 //操作的掩码
}TByteNode;

// 设置Short结点
typedef struct
{
	unsigned int  m_uiKey;	       //操作的KEY值
	unsigned short m_usData;       //操作的数据
	unsigned short m_usMask;       //操作的掩码
}TShortNode;

// 设置Int结点
typedef struct
{
	unsigned int  m_uiKey;	       //操作的KEY值
	unsigned int m_uiData;         //操作的数据
	unsigned int m_uiMask;         //操作的掩码
}TIntNode;

// 设置Long结点
typedef struct
{
	unsigned int  m_uiKey;	       //操作的KEY值
	unsigned long long m_ulData;   //操作的数据
	unsigned long long m_ulMask;   //操作的掩码
}TLongNode;

// 更新操作结果结点
typedef struct
{
	short m_sResultCode;           //该号码的操作结果
	unsigned int  m_uiKey;
}TRspNode;

typedef struct
{
	unsigned int  m_uiParaValue[32];	    
}Para_En;

// Server类型
enum SERVER_TYPE
{
	SERVER_READ		= 0, //读机器
	SERVER_WRITE 	= 1, //读写机器
	SERVER_INVALID 	= -1,
};

// 全量数据操作类型
enum OPT_TYPE
{
	OPT_GET 		 = 0,
	OPT_GET_EN = 1,
	OPT_INSERT 		 = 2,
	OPT_UPDATE 		 = 3,
	OPT_DELETE 		 = 4,
	OPT_INVALID 	 = -1,
};

// 小粒度操作类型
enum SET_TYPE
{
	SET_BIT			 = 0,
	SET_BYTE		 = 1,
	SET_SHORT		 = 2,
	SET_INT			 = 3,
	SET_LONG		 = 4,
	SET_INVALID		 = -1,
};

#pragma pack()
/*************************************************************
*函数名称: getDelay()
*输入参数: start ----起始时间 end----结束时间
*返回参数: 时间差，单位为ms
*函数功能: 获取时间差
************************************************************/
int getDelay(const struct timeval & start, const struct timeval & end);

class CBitService
{
	public:
		CBitService();
		virtual ~CBitService();
		
	public:
		/*
		 * desc: 使用api前,先要进行相关的初始化
		 * @输入参数：
		 * 		iModId: L5的被调模块编码
		 * 		iCmd:  L5的被调接口编码
		 *		iServerType: 无意义，默认填0
		 *		iTimeOut:
		*/
		int Init(int iModId, int iCmd, int iServerType, int iTimeOut);
		
		/*
		 * desc: 获取错误信息
		*/
		const char* GetErrMsg();
		
		/*
		 * des: 异步打包api
		 *
		 * @输入参数：
		 * 		bid：业务ID
		 * 		opt：操作类型
		 * 		v_keynode: key列表各key对应的数据
		 * 		buffer：数据缓冲区，空间在外部由用户申请
		 * 		max_buffer_len：buffer缓冲区的大小
		 *
		 * @输出参数：
		 * 		buffer: 打包完成后的数据区
		 * 		buffer_len：打包后，包体的实际大小
		 * 		return value: 操作是否成功
		*/
		int Encode(unsigned short bid, int opt, vector<TKeyNode> &v_keynode, char * buffer, int max_buffer_len, int &buffer_len, unsigned int uiBitSign = 0);
		
		/*
		 * des: 异步解包api
		 *
		 * @输入参数：
		 * 		buffer
		 *  	buffer_len
		 *
		 * @输出参数：
		 * 		bid
		 * 		opt
		 * 		v_keynode
		 * 		return value: 操作是否成功（各key操作的结果要详细解析v_keynode）
		*/
		int Decode(char *buffer, int buffer_len, vector<TResultNode> &v_resultnode,Para_En* pParaValue = NULL);
		
	public:
		/*
		 * @desc: 异步setbit打包函数
		 * 
		 * @输入参数:
		 *			bid
		 *			from: 从第几个bit位开始, 起始bit从0 开始计数
		 *		    datalen: 要更新的bit位的个数, 起始从1 开始
		 *			v_bitnode: 需要更新的数据
		 *			buffer: 输入数据区
		 *			max_buffer_len: 输入数据区的最大长度
		 *
		 * @输出参数:
		 *			buffer: 打包好的数据区
		 *			buffer_len: 打包后的数据区长度
		*/
		int SetBit_Encode(unsigned short bid, unsigned short from, unsigned short datalen, vector<TBitNode> &v_bitnode, char * buffer, int max_buffer_len, int &buffer_len);

		/*
		 * @desc: 异步setbit解包函数
		 * 
		 * @输入参数:
		 * 			buffer:需要解包的数据区
		 *			buffer_len: 数据区长度
		 * @输出参数:
		 *			v_nodesrp:setbit的结果
		 *			return value: 成功返回0,其他值为对应的错误码
		*/
		int SetBit_Decode(char *buffer, int buffer_len, vector <TRspNode> &v_nodersp);
		
		// set byte
		int SetByte_Encode(unsigned short bid, unsigned short from, vector<TByteNode> &v_bitnode, char *buffer, int max_buffer_len, int &buffer_len);
		int SetByte_Decode(char *buffer, int buffer_len, vector <TRspNode> &v_nodersp);
		
		// set short
		int SetShort_Encode(unsigned short bid, unsigned short from, vector<TShortNode> &v_bitnode, char *buffer, int max_buffer_len, int &buffer_len);
		int SetShort_Decode(char *buffer, int buffer_len, vector <TRspNode> &v_nodersp);
		
		// int
		int SetInt_Encode(unsigned short bid, unsigned short from, vector<TIntNode> &v_bitnode, char *buffer, int max_buffer_len, int &buffer_len);
		int SetInt_Decode(char *buffer, int buffer_len, vector <TRspNode> &v_nodersp);
		
		// long
		int SetLong_Encode(unsigned short bid, unsigned short from, vector<TLongNode> &v_bitnode, char *buffer, int max_buffer_len, int &buffer_len);
		int SetLong_Decode(char *buffer, int buffer_len, vector <TRspNode> &v_nodersp);
		
	public:

		/*
		 * desc: 同步获取key对应的值
		 * @input:
		 *		realtime: 	默认非一致性查询, realtime>0时为一致性查询
		 *		bid: 		业务id
		 *		v_keys:		查询请求key列表
		 *		v_keynode:  查询结果结点列表
		 *	
		 * @output:
		 *		v_keynode: 查询结果
		 *		return value:
		*/
		int Get(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode, int realtime = 0);

		/*
		 * Get_En: 增强查询类命令
		*/
		int Get_En(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode,unsigned int uiBitSign, Para_En* pParaValue, int realtime = 0);

		/*
		 * desc: 同步更新key列表对应的值
		*/
		int Update(unsigned short bid, vector<TKeyNode> &v_keynode, vector<TResultNode> &v_resultnode);

		/*
		 * desc: 同步插入记录
		*/
		int Insert(unsigned short bid, vector<TKeyNode> &v_keynode, vector<TResultNode> &v_resultnode);

		/*
		 * desc: 同步删除记录
		*/
		int Delete(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode);
	
	public:
		int SetBit(unsigned short bid, unsigned short from, unsigned short datalen, vector<TBitNode> &v_bitnode, vector<TRspNode> &v_rspnode);
		int SetByte(unsigned short bid, unsigned short from, vector<TByteNode> &v_bytenode, vector<TRspNode> &v_rspnode);
		int SetShort(unsigned short bid, unsigned short from, vector<TShortNode> &v_shortnode, vector<TRspNode> &v_rspnode);
		int SetInt(unsigned short bid, unsigned short from, vector<TIntNode> &v_intnode, vector<TRspNode>&v_rspnode);
		int SetLong(unsigned short bid, unsigned short from, vector<TLongNode> &v_longnode, vector<TRspNode> &v_rspnode);
		
	private:
		
		MapIpSocket m_mapSocket;				//与Server的socket连接列表

		int m_iModId;							//被调用模块编码
		int m_iCmd;								//被调接口编码
		
		char  m_szErrMessage[MAX_ERR_MSG_LEN];	// 失败日志记录buffer
		
		char * m_szSndBuf;	 					// 同步请求buf
		char * m_szRcvBuf;   					// 同步响应buf

		char m_szLocalName[MAX_NAMESER_LEN];		// 客户端应用名字
		char m_szServerName[MAX_NAMESER_LEN];		// 请求server名字
		int m_iServerType;						// 请求server的类型(读机器、写机器)
		
		int m_iTimeOut;							// server超时时间
		
		int Set_Decode(int set_type, char * buffer, int buffer_len, vector <TRspNode> & v_rspnode);

		int SendAndRecvByLocalConnect(char * send_buf, int send_len, char * recv_buf, int recv_maxLen, int &recv_len);
		
};

};

#endif
