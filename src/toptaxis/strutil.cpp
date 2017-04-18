/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*
 * =====================================================================================
 *
 *       Filename:  strutil.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/24/2009 06:03:09 AM
 *       Revision:  none:
 *       Compiler:  gcc
 *
 *         Author:   (skb), 
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "strutil.h"

using namespace std;

void CStrUtil::SplitStringUsing(const string& full, const string delim,
					  vector<string>* result)
{
	(*result).clear();

	if(full.size() == 0)
	{
		return;
	}
	int iFirst = 0, iIndex = 0;
	while(true)
	{
		iIndex = full.find_first_of(delim, iFirst);
		if( (int)string::npos == iIndex )
		{
			if( (unsigned)iFirst <= full.size()-1)
			{
				(*result).push_back( full.substr(iFirst, full.size()-iFirst) );
			}
			return;
		}
		(*result).push_back( full.substr(iFirst, iIndex-iFirst) );
		iFirst = iIndex + (int)delim.size();
	}
}

