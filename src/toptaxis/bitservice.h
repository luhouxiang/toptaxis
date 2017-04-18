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

#define MAX_NAMESER_LEN 		256		//���ֳ���
#define MAX_BATCH_KEYS_NUM	100		//�������������Ŀ
#define MAX_MSG_LEN 		4096	//������󳤶�
#define MAX_BITSET_LEN 		32		//����λ��󳤶�
#define MAX_ERR_MSG_LEN 	256		//������Ϣ��󳤶�
#define MAX_IP_ADDR_LEN 	16		//IP��ַ����

#pragma pack(1)
typedef map<string, void*> MapIpSocket;

// ȫ�����ݲ�����Ӧ���
typedef struct
{
	short m_sResultCode;  //0��ʾ�����ɹ���������ʾstore���������صĴ�����
	unsigned int m_uiKey; //��
	string m_strData;	  //��������
}TResultNode;

// ȫ�����ݲ����Ľ��
typedef struct
{
	unsigned int m_uiKey; //��
	string m_strData;	  //��������
}TKeyNode;

// ����Bit���
typedef struct
{
	unsigned int  m_uiKey;	       //������KEYֵ
	unsigned int m_uiData;
}TBitNode;

// ����Byte���
typedef struct
{
	unsigned int  m_uiKey;	       //������KEYֵ
	char m_szData;                 //����������
	char m_szMask;                 //����������
}TByteNode;

// ����Short���
typedef struct
{
	unsigned int  m_uiKey;	       //������KEYֵ
	unsigned short m_usData;       //����������
	unsigned short m_usMask;       //����������
}TShortNode;

// ����Int���
typedef struct
{
	unsigned int  m_uiKey;	       //������KEYֵ
	unsigned int m_uiData;         //����������
	unsigned int m_uiMask;         //����������
}TIntNode;

// ����Long���
typedef struct
{
	unsigned int  m_uiKey;	       //������KEYֵ
	unsigned long long m_ulData;   //����������
	unsigned long long m_ulMask;   //����������
}TLongNode;

// ���²���������
typedef struct
{
	short m_sResultCode;           //�ú���Ĳ������
	unsigned int  m_uiKey;
}TRspNode;

typedef struct
{
	unsigned int  m_uiParaValue[32];	    
}Para_En;

// Server����
enum SERVER_TYPE
{
	SERVER_READ		= 0, //������
	SERVER_WRITE 	= 1, //��д����
	SERVER_INVALID 	= -1,
};

// ȫ�����ݲ�������
enum OPT_TYPE
{
	OPT_GET 		 = 0,
	OPT_GET_EN = 1,
	OPT_INSERT 		 = 2,
	OPT_UPDATE 		 = 3,
	OPT_DELETE 		 = 4,
	OPT_INVALID 	 = -1,
};

// С���Ȳ�������
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
*��������: getDelay()
*�������: start ----��ʼʱ�� end----����ʱ��
*���ز���: ʱ����λΪms
*��������: ��ȡʱ���
************************************************************/
int getDelay(const struct timeval & start, const struct timeval & end);

class CBitService
{
	public:
		CBitService();
		virtual ~CBitService();
		
	public:
		/*
		 * desc: ʹ��apiǰ,��Ҫ������صĳ�ʼ��
		 * @���������
		 * 		iModId: L5�ı���ģ�����
		 * 		iCmd:  L5�ı����ӿڱ���
		 *		iServerType: �����壬Ĭ����0
		 *		iTimeOut:
		*/
		int Init(int iModId, int iCmd, int iServerType, int iTimeOut);
		
		/*
		 * desc: ��ȡ������Ϣ
		*/
		const char* GetErrMsg();
		
		/*
		 * des: �첽���api
		 *
		 * @���������
		 * 		bid��ҵ��ID
		 * 		opt����������
		 * 		v_keynode: key�б��key��Ӧ������
		 * 		buffer�����ݻ��������ռ����ⲿ���û�����
		 * 		max_buffer_len��buffer�������Ĵ�С
		 *
		 * @���������
		 * 		buffer: �����ɺ��������
		 * 		buffer_len������󣬰����ʵ�ʴ�С
		 * 		return value: �����Ƿ�ɹ�
		*/
		int Encode(unsigned short bid, int opt, vector<TKeyNode> &v_keynode, char * buffer, int max_buffer_len, int &buffer_len, unsigned int uiBitSign = 0);
		
		/*
		 * des: �첽���api
		 *
		 * @���������
		 * 		buffer
		 *  	buffer_len
		 *
		 * @���������
		 * 		bid
		 * 		opt
		 * 		v_keynode
		 * 		return value: �����Ƿ�ɹ�����key�����Ľ��Ҫ��ϸ����v_keynode��
		*/
		int Decode(char *buffer, int buffer_len, vector<TResultNode> &v_resultnode,Para_En* pParaValue = NULL);
		
	public:
		/*
		 * @desc: �첽setbit�������
		 * 
		 * @�������:
		 *			bid
		 *			from: �ӵڼ���bitλ��ʼ, ��ʼbit��0 ��ʼ����
		 *		    datalen: Ҫ���µ�bitλ�ĸ���, ��ʼ��1 ��ʼ
		 *			v_bitnode: ��Ҫ���µ�����
		 *			buffer: ����������
		 *			max_buffer_len: ��������������󳤶�
		 *
		 * @�������:
		 *			buffer: ����õ�������
		 *			buffer_len: ����������������
		*/
		int SetBit_Encode(unsigned short bid, unsigned short from, unsigned short datalen, vector<TBitNode> &v_bitnode, char * buffer, int max_buffer_len, int &buffer_len);

		/*
		 * @desc: �첽setbit�������
		 * 
		 * @�������:
		 * 			buffer:��Ҫ�����������
		 *			buffer_len: ����������
		 * @�������:
		 *			v_nodesrp:setbit�Ľ��
		 *			return value: �ɹ�����0,����ֵΪ��Ӧ�Ĵ�����
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
		 * desc: ͬ����ȡkey��Ӧ��ֵ
		 * @input:
		 *		realtime: 	Ĭ�Ϸ�һ���Բ�ѯ, realtime>0ʱΪһ���Բ�ѯ
		 *		bid: 		ҵ��id
		 *		v_keys:		��ѯ����key�б�
		 *		v_keynode:  ��ѯ�������б�
		 *	
		 * @output:
		 *		v_keynode: ��ѯ���
		 *		return value:
		*/
		int Get(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode, int realtime = 0);

		/*
		 * Get_En: ��ǿ��ѯ������
		*/
		int Get_En(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode,unsigned int uiBitSign, Para_En* pParaValue, int realtime = 0);

		/*
		 * desc: ͬ������key�б��Ӧ��ֵ
		*/
		int Update(unsigned short bid, vector<TKeyNode> &v_keynode, vector<TResultNode> &v_resultnode);

		/*
		 * desc: ͬ�������¼
		*/
		int Insert(unsigned short bid, vector<TKeyNode> &v_keynode, vector<TResultNode> &v_resultnode);

		/*
		 * desc: ͬ��ɾ����¼
		*/
		int Delete(unsigned short bid, vector<unsigned int> &v_keys, vector<TResultNode> &v_resultnode);
	
	public:
		int SetBit(unsigned short bid, unsigned short from, unsigned short datalen, vector<TBitNode> &v_bitnode, vector<TRspNode> &v_rspnode);
		int SetByte(unsigned short bid, unsigned short from, vector<TByteNode> &v_bytenode, vector<TRspNode> &v_rspnode);
		int SetShort(unsigned short bid, unsigned short from, vector<TShortNode> &v_shortnode, vector<TRspNode> &v_rspnode);
		int SetInt(unsigned short bid, unsigned short from, vector<TIntNode> &v_intnode, vector<TRspNode>&v_rspnode);
		int SetLong(unsigned short bid, unsigned short from, vector<TLongNode> &v_longnode, vector<TRspNode> &v_rspnode);
		
	private:
		
		MapIpSocket m_mapSocket;				//��Server��socket�����б�

		int m_iModId;							//������ģ�����
		int m_iCmd;								//�����ӿڱ���
		
		char  m_szErrMessage[MAX_ERR_MSG_LEN];	// ʧ����־��¼buffer
		
		char * m_szSndBuf;	 					// ͬ������buf
		char * m_szRcvBuf;   					// ͬ����Ӧbuf

		char m_szLocalName[MAX_NAMESER_LEN];		// �ͻ���Ӧ������
		char m_szServerName[MAX_NAMESER_LEN];		// ����server����
		int m_iServerType;						// ����server������(��������д����)
		
		int m_iTimeOut;							// server��ʱʱ��
		
		int Set_Decode(int set_type, char * buffer, int buffer_len, vector <TRspNode> & v_rspnode);

		int SendAndRecvByLocalConnect(char * send_buf, int send_len, char * recv_buf, int recv_maxLen, int &recv_len);
		
};

};

#endif
