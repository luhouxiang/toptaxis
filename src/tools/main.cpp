/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include <stdio.h>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tcp_client.h"

using namespace std;

#pragma pack(1)
struct ProHdr
{
	char ver;
	unsigned short len;
};
#pragma pack()

int main(int argc,char** argv)
{
	if(4!=argc)
	{
		printf("Usage: %s ip port key-value\n",argv[0]);
		exit(-1);
	}

	int iRet=0;
	ProHdr Header;
	string strData;
	string strIP=argv[1];
	unsigned short usPort=atoi(argv[2]);
	
	//构造数据
	strData=argv[3];
	
	Header.ver=1;	
	Header.len=htons(strData.size());
	fprintf(stderr,"send data[%s] size[%u]\n",strData.c_str(),(unsigned)strData.size());
	
	char* pszPack=new char[sizeof(ProHdr)+strData.size()];
	memcpy(pszPack,&Header,sizeof(Header));
	memcpy(pszPack+sizeof(Header),strData.c_str(),strData.size());

	char szErrMsg[256];
	memset(szErrMsg,0,256);


	//短链接
	CTCPClientSocket m_Conn;
	if(m_Conn.InitServer(strIP.c_str(), usPort, 3, szErrMsg, 256))
	{
		fprintf(stderr,"TimingWorker InitServer[%s:%u]error[%s]",strIP.c_str(),usPort,szErrMsg);
		return -1;
	}

	iRet = m_Conn.SendDataWithoutRecv(pszPack, sizeof(ProHdr)+strData.size(), szErrMsg, 256);
	if(iRet != 0)
	{
		fprintf(stderr,"SendDataWithoutRecv[%s:%u]error[%s]",strIP.c_str(),usPort,szErrMsg);
		return -1;
	}
	
	return 0;
}
