/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include "comm_tools.h"


/*************************************************************
*函数名称: getDelay()
*输入参数: start ----起始时间 end----结束时间
*返回参数: 时间差，单位为ms
*函数功能: 获取时间差
************************************************************/
int GetDelay(const struct timeval & start, const struct timeval & end)
{
        //timeval 由两部分组成，tv_sec 表示秒 级别，tv_usec 表示 微妙级别
	int delay= ( end.tv_sec - start.tv_sec)*1000000+end.tv_usec-start.tv_usec;
	//将时间四舍五入
	delay = (delay + 500)/1000;
	return delay>=0?delay:0 ;
}



/*************************************************************
*函数名称: TextToMap()
*输入参数: pchString: 文本消息    
                            pchSplitter:文本消息间隔符号
                            mapResult:存储解析结果
*返回参数: -1:失败；0:成功
*函数功能: 解析文本消息，存储在map 中
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
		//获得一个片段(xxx=xxx)
		string strToken(pchStart, pchEnd-pchStart);
		string::size_type idx = strToken.find('=');
		if ( string::npos == idx )
		{
			//没有=, 就存{"name", ""}
			mapResult.insert( make_pair(strToken, string("")) );
		}
		else
		{
			//有=, 也要判断一下有没有value
			if ( idx >= strToken.size()-1 )
			{
				//没有value
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


