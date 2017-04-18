/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include "comm_tools.h"


/*************************************************************
*��������: getDelay()
*�������: start ----��ʼʱ�� end----����ʱ��
*���ز���: ʱ����λΪms
*��������: ��ȡʱ���
************************************************************/
int GetDelay(const struct timeval & start, const struct timeval & end)
{
        //timeval ����������ɣ�tv_sec ��ʾ�� ����tv_usec ��ʾ ΢���
	int delay= ( end.tv_sec - start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	//��ʱ����������
	delay = (delay + 500)/1000;
	return delay>=0?delay:0 ;
}



/*************************************************************
*��������: TextToMap()
*�������: pchString: �ı���Ϣ    
                            pchSplitter:�ı���Ϣ�������
                            mapResult:�洢�������
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: �����ı���Ϣ���洢��map ��
************************************************************/
 int TextToMap( const char* pchString, const char* pchSplitter, map<string,string>& mapResult )
{
	const char* pchStart, *pchEnd;
	int iSplitterLen = strlen(pchSplitter);

	mapResult.clear();

        pchStart = pchString;
        pchEnd = strstr(pchStart, pchSplitter);

	while ( pchEnd!=NULL && (*pchStart) != '\0' )
	{
		//���һ��Ƭ��(xxx=xxx)
		string strToken(pchStart, pchEnd-pchStart);
		string::size_type idx = strToken.find('=');
		if ( string::npos == idx )
		{
			//û��=, �ʹ�{"name", ""}
			mapResult.insert( make_pair(strToken, string("")) );
		}
		else
		{
			//��=, ҲҪ�ж�һ����û��value
			if ( idx >= strToken.size()-1 )
			{
				//û��value
				mapResult.insert( make_pair(strToken.substr(0, idx), string("")) );
			}
			else
			{
				mapResult.insert( make_pair(strToken.substr(0, idx), strToken.substr(idx+1)) );
			}
		}

		pchStart = pchEnd + iSplitterLen;
		if ( pchStart != '\0' )
		{
			pchEnd = strstr(pchStart, pchSplitter);
		}
        }

        return 0;
}    


