/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: toptaxis_backup.h
*      Author: 
*      Copyright: Copyright@2011 tencent
*      Date: 2011/8/31
*      Description: TOP排序系统系统数据备份恢复
*      History: 1.   2011/8/31   Ver1.0  build this moudle
*******************************************************************************/
#ifndef _TOPTAXIS_BACKUP_H_
#define _TOPTAXIS_BACKUP_H_
#include <vector>
#include <string>
#include <map>
#include <set>
#include "tcp_client.h"

using namespace std;
#define  TAXIS_COLD_LOG_FILENAME              256
class CColdLog
{
private:
    /* log path*/
    char m_szLogPath[TAXIS_COLD_LOG_FILENAME];
    /*log file fd*/
    int m_iLogfd;
public:
    /* 
     * default ctor
    */
    CColdLog();
    /* 
     * default ctor
    */
    virtual ~CColdLog();

    /*
     * init all the params before write water log
     * pass the address and set initial value
    */
    int Init(char *szlogpath);
    /*
     * check water log logic and write data to log file if necessary
     *
     * @input: data: data need to write log
     * datalen: len of data
     *   re_file: file name in the request message
     *   re_line:line num in the request message
     *re_offset:  offset in the request message
     *
     * @output: re_file:file name in the response message
     *    re_line: line num in the response message
     * re_offset: offset in the response message
     *
     * @return value: error no
    */
    int WriteLog(const char * data, unsigned int datalen);
    void CloseFile();
};

int RecoverData(void);
int CrotabUpdateHistoryData(void);  
int CrotabColdDataBackup(void);
#endif


