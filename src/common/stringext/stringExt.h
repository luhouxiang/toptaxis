/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/* $Id$
 * This is unpublished proprietary source code of Tencent Ltd.
 * The copyright notice above does not evidence any actual or intended publication of such source code
 *
 * NOTICE: UNAUTHORIZED DISTRIBUTION, ADAPTATION OR USE MAY BE
 * SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *
 * FILE:           
 *        stringExt.h
 * DESCRIPTION:    
 *        ×Ö·û´®À©Õ¹
 * AUTHOR:
 *         - 2009-06-29
 * MODIFIED:
 */ 
#ifndef __STRING_EXTENT_H__
#define __STRING_EXTENT_H__

#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

namespace StrExt
{
    int str2Vect(const char* sSrc, vector<string> &vsDest, const char *sep);
    int str2Map(const char *sSrc, map<string,string> & msDest, const char *sep1,const char *sep2);
};

#endif


