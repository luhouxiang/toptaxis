/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称:  stringExt.cpp
 *       文件功能:  将string字符串切割为vector或map
 *       文件作者:   
 *       修改时间:  
 *       修改记录:  
 ***************************************************************************/
 #include "stringExt.h"
 
 namespace StrExt
 {
     /*************************************************************************** 
     *       文件名称:  stringExt.cpp
     *       文件功能:  将string字符串切割为vector
     *       文件作者:   
     *       修改时间:  
     *       修改记录:  
     ***************************************************************************/
     int str2Vect(const char* pSrc, vector<string> &vDest, const char *pSep=",")
     {  
        if(NULL == pSrc)
        {
            return -1;
        }
        
        int iLen= strlen(pSrc);
        if(iLen==0)
        {
            return -1;
        }

        char *pTmp= new char[iLen + 1];
        if(pTmp==NULL)
        {
            return -1;
        }

        memcpy(pTmp, pSrc, iLen);
        pTmp[iLen] ='\0';

        char *pCurr = strtok(pTmp,pSep);
        while(pCurr)
        {
            vDest.push_back(pCurr);
            pCurr = strtok(NULL, pSep);
        }

        delete[] pTmp; 
        return 0;
    }

    /*************************************************************************** 
     *       文件名称:  stringExt.cpp
     *       文件功能:  将string字符串切割为map
     *       文件作者:   
     *       修改时间:  
     *       修改记录:  
     ***************************************************************************/
    int str2Map(const char *pSrc, map<string,string> & msDest, const char *sep1,const char *sep2)
    {
        vector<string> vsPair;
        if(str2Vect(pSrc, vsPair, sep1)) 
        {
            return -1;
        }

        for(int i=0;i<(int)vsPair.size();i++)
        {
            vector<string> vsTemp;
            if((str2Vect(vsPair[i].c_str(), vsTemp, sep2)==0)
                    &&(vsTemp.size()>1))
            {
                msDest[vsTemp[0]] = vsTemp[1];
            }
        }
        return 0;
    }
}; 
