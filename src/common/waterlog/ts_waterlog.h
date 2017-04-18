/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: ts_waterlog.h
*        Author: 
*     Copyright: Copyright@2011 tencent
*          Date: 2011/9/26
*   Description: TS系统写流水文件
*       History: 1.   2011/9/26   Ver1.0  build this moudle
*******************************************************************************/

#ifndef __TS_WATERLOG_H__
#define __TS_WATERLOG_H__

#define MAX_LOG_CNT           1000
#define MAX_FILENAME_LEN     256
#define MAX_LOGDIR_LEN       256
#define MAX_PRENAME_LEN      32
#define MAX_LOG_SIZE         (1 << 31)

#include <time.h>

class CTsWaterLog
{
public:

    CTsWaterLog();

    virtual ~CTsWaterLog();

    static CTsWaterLog* Instance(); 

    int Init(char *pLogDir, char *pLogPreName, unsigned int uiLogsize);

    void LogFileName(unsigned int uiSeq, char* pFileName, time_t now);

    unsigned int GetLogSeq();

    int OpenFd(time_t now);

    int ShiftFd (time_t now);

    void WriteWaterLog (const char *pWaterLog, unsigned int uiLogLen);

private:

    static CTsWaterLog *m_pInstance;

    char m_szLogDir[MAX_LOGDIR_LEN];

    char m_szLogPreName[MAX_PRENAME_LEN];

    unsigned int m_uiLogSize;

    unsigned int m_uiSeq;

    int m_iOpFd;

    unsigned int m_uiDay;

};

#endif

