/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef __WARN_H__
#define __WARN_H__

#include <string>
using namespace std;

#define WARN_CONF_DIR "../conf/warn.xml"


class CWarnConf
{
public:
	CWarnConf();
	~CWarnConf();

	static CWarnConf* Instance(); 		

	int ReadCfg(const char* pConfigFilePath);
	void PrintLog();
	int SendWarn(string strWarnMsg);
	

public:
	static CWarnConf * m_Instance;

private:
	bool m_bFlag;
	int m_iWarnID;
	int m_iRetryTime;
};

#endif


