/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_main.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP排序系统的函数声明
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_MAIN_HEADER_H_
#define _TOPTAXIS_MAIN_HEADER_H_
#include <vector>
#include <string>
#include <map>
#include <set>
#include "tcp_client.h"
using namespace std;

#define MAX_CPU_NO                       16 
/*进程退出种类*/
enum EXIT_MODE
{
    E_EXIT_NORMAL=  0,
    E_EXIT_MANUAL = 170
};

int InitApp();
int StartThread();
int EnterWork();
#endif


