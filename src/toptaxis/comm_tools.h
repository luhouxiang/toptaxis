/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _COMM_TOOLS_H_
#define _COMM_TOOLS_H_

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <string>
#include <map>
using namespace std;


extern  int TextToMap( const char* pchString, const char* pchSplitter, map<string,string>& mapResult );
extern int GetDelay(const struct timeval & start, const struct timeval & end);

#endif
