/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _TCP_TEXT_CLIENT_H
#define  _TCP_TEXT_CLIENT_H

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

using namespace std;

#define MAX_LEVEL_IP_LEN 16

#define MAX_UNSIGNED_INT_LEN 4

/****************************************************************
* API      ����:  ��װTCP ͨѶЭ��
*
*��Ϣ��ʽ:  ��������Ϣ��������Ϣ��ǰ���ĸ��ֽڴ�����Ϣ�ĳ���
*
*ע�ⷽ��:  �����ʹ�ó����ӣ��뽫CTCPClientSocket ����Ϊȫ����
*
*                           ������ڶ��߳��е��ã������߳��ڲ������CTCPClientSocket ��
*
*�ο�����: ��test.cpp
****************************************************************/
enum RETURN_CODE_DEF
{
	LEVEL_RETURN_SUCCESS = 0,
	LEVEL_RETURN_FAILED = -1,	
	LEVEL_SYSTEM_ERROR =-2,
};

class CTCPClientSocket
{
	public:
		CTCPClientSocket();
		
		~CTCPClientSocket();


		int InitServer(const char* szIP, unsigned short usPort, unsigned int iTimeOut, char* szErrorMsg, unsigned int iMaxErrorLen); 

		/*��ʼ���Զ�Server,      ����0 :�ɹ��� 1: ʧ��*/
		int SendAndRcvData(char* szReqBuf, unsigned int iReqLen, char* szRspBuf, unsigned int iRspMaxLen, char* szErrorMsg, unsigned int iMaxErrorLen); 

		int SendDataWithoutRecv(const char* szReqBuf, unsigned int iReqLen, char* szErrorMsg, unsigned int iMaxErrorLen);


		/*ʧ�ܺ������ӿ�*/
		int Reconnect(char* szErrorMsg, unsigned int iMaxErrorLen); 

		int CloseSocket();


	private:
		int CreateAndConnect(char* szErrorMsg, int iMaxErrorLen);
		

		
		

		int SendData(const char* szReqBuf, unsigned int iReqLen,char* szErrorMsg, unsigned int iMaxErrorLen);
		
		int RecvData(char* szRspBuf, unsigned int iRspMaxLen, unsigned int iNeedLen, char*  szErrorMsg, unsigned int  iMaxErrorLen); 

		int JudgeEnd(char* szRspBuf, unsigned int iMsgLen);


		int IsConnected( fd_set *rdSet, fd_set *wrSet, fd_set *exSet );
		

	
      public:
		char m_szIP[MAX_LEVEL_IP_LEN];  /*Զ��IP*/

		unsigned short m_usPort;               /*Զ��Port*/
     private:
		unsigned int m_iTimeOut;                             /*��ʱʱ��*/

		unsigned int m_iBlockMode;                          /*Ĭ��Ϊ������ʽ*/

		int m_iSock;
		
};


#endif
