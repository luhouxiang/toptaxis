/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: ts_waterlog.cpp
*        Author: 
*     Copyright: Copyright@2011 tencent
*          Date: 2011/9/26
*   Description: TS系统写流水文件
*       History: 1.   2011/9/26   Ver1.0  build this moudle
*******************************************************************************/

#include "ts_waterlog.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#define DEBUG_ARGS __FUNCTION__, __LINE__
#define DEBUG_FAMT "FUNCTION[%s]LINE[%d]"

/*多线程写流水日志线程锁*/
pthread_mutex_t g_MutexLog;

CTsWaterLog * CTsWaterLog::m_pInstance = NULL;

/*******************************************************************************
* Description: 构造函数
*       Input: 无               
*      Output: 无
*      Return: 无
*      Others: 无
*******************************************************************************/
CTsWaterLog::CTsWaterLog()
{
    m_iOpFd = -1;   
    memset(m_szLogPreName, 0, sizeof(m_szLogPreName));
}

/*******************************************************************************
* Description: 析构函数
*       Input: 无               
*      Output: 无
*      Return: 无
*      Others: 无
*******************************************************************************/
CTsWaterLog::~CTsWaterLog()
{
    if(NULL != m_pInstance)	
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }  
}

/*******************************************************************************
* Description: 获取句柄
*       Input: 无               
*      Output: 无
*      Return: 句柄
*      Others: 无
*******************************************************************************/
CTsWaterLog *CTsWaterLog::Instance()
{
    if(NULL == m_pInstance)
    {
        m_pInstance = new CTsWaterLog;
    }
    return m_pInstance;
}

/*******************************************************************************
* Description: 依照当前的日期和SEQ生成文件名
*       Input: uiSeqArg:seq      
*         now: 当前时间
*      Output: pFileName: 生成的文件名
*      Return: 无
*      Others: 无
*******************************************************************************/
void CTsWaterLog::LogFileName(unsigned int uiSeqArg, char* pFileName, time_t now)
{
    struct tm stTime;

    localtime_r(&now, &stTime);

    snprintf (pFileName, MAX_FILENAME_LEN - 1, "%s/%s%04d%02d%02d%03d", m_szLogDir, m_szLogPreName, 
    stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, uiSeqArg);
}

/*******************************************************************************
* Description: 获取流水日志当日最新的日志SEQ,没有日志则为0
*       Input: 无
*      Output: 无
*      Return: 当日最新日志的SEQ
*      Others: 无
*******************************************************************************/
unsigned int CTsWaterLog::GetLogSeq()
{
    char szFileName[MAX_FILENAME_LEN];

    time_t now = time(NULL);
    
    unsigned int uiSeqTmp;
    for(uiSeqTmp = 0; uiSeqTmp < MAX_LOG_CNT; uiSeqTmp++)
    {
        /*按照当前日期和SEQ生成文件名*/
        LogFileName(uiSeqTmp, szFileName, now);

        /*判断该文件是否存在*/
        if (access (szFileName, F_OK)) 
        {
            if (0 == uiSeqTmp)
            {
                return 0;
            }
            else
            {
                break;
            }
        }
    }

    /*当日日志数超过限制*/
    if (MAX_LOG_CNT == uiSeqTmp) 
    {
        printf ("[ERROR]"DEBUG_FAMT"\n", DEBUG_ARGS);
        return -1;
    }
    
    /*返回不存在的SEQ的上一个SEQ，若当日没有流水日志则返回0*/
    return uiSeqTmp == 0 ? uiSeqTmp : uiSeqTmp - 1;    
}

/*******************************************************************************
* Description: 初始化流水日志类
*       Input: pLogDir:日志目录
*          pLogPreName:日志前缀
*            uiLogSize:日志记录最大值 
*      Output: 无
*      Return: 0/-1
*      Others: 无
*******************************************************************************/
int CTsWaterLog::Init(char *pLogDir, char *pLogPreName, unsigned int uiLogSize)
{

    if (uiLogSize >= (unsigned int)MAX_LOG_SIZE) 
    {
        printf ("[ERROR]"DEBUG_FAMT"LOGSIZE[%u]\n", DEBUG_ARGS, uiLogSize);
        return -1;
    }

    if (access (pLogDir, W_OK)) 
    {
        printf ("[ERROR]"DEBUG_FAMT"LOGDIR[%s]\n", DEBUG_ARGS, pLogDir);
        return -1;
    }

    strncpy(m_szLogDir, pLogDir, sizeof (m_szLogDir) - 1);
    
    if (pLogPreName != NULL)
    {
        strncpy (m_szLogPreName, pLogPreName, sizeof (m_szLogPreName) - 1);
    }
    
    m_uiLogSize = uiLogSize;

    pthread_mutex_init(&g_MutexLog, NULL);
      
    return 0;
}

/*******************************************************************************
* Description: 打开当日最新的日志
*       Input: now:当天时间
*      Output: m_iOpFd:日志文件FD
*      Return: m_iOpFd:日志文件FD
*      Others: 无
*******************************************************************************/
int CTsWaterLog::OpenFd(time_t now)
{
    char szFileName[MAX_FILENAME_LEN];

    /*依照当日最新的SEQ生成文件名*/
    LogFileName (m_uiSeq, szFileName, now);

    /*获取文件描述符，不存在则创建文件，文件打开后需要记录文件描述符*/
    m_iOpFd = open (szFileName, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if(m_iOpFd < 0)
    {
        printf ("[ERROR]"DEBUG_FAMT"OPFD[%d]\n", DEBUG_ARGS, m_iOpFd);
        return m_iOpFd;
    }

    /*记录文件打开时间*/
    struct tm *pTime = localtime(&now);
    m_uiDay = (unsigned int)pTime->tm_yday;
    
    if (m_iOpFd > 0) 
    {
        int iVal;
        iVal = fcntl(m_iOpFd, F_GETFD, 0);
        iVal |= FD_CLOEXEC;
        fcntl(m_iOpFd, F_SETFD, iVal);
    }
    return m_iOpFd;
}

/*******************************************************************************
* Description: 判断文件大小是否超过最大值，若大于最大值，则新建文件
*       Input: now:当天时间
*      Output: m_uiSeq:日志文件SEQ
*      Return: m_iOpFd:日志文件FD
*      Others: 无
*******************************************************************************/
int CTsWaterLog::ShiftFd (time_t now)
{    
    /*先前未打开过日志文件，则打开最新的文件，若文件不存在则创建文件*/
    if (m_iOpFd < 0) 
    {
        /*获取当日日志文件最新的SQE，不存在文件则为0*/
        m_uiSeq = GetLogSeq();
        if((unsigned int)-1 == m_uiSeq)
        {
            printf ("[ERROR]"DEBUG_FAMT"\n", DEBUG_ARGS);
            return -1;
        }

        if(OpenFd(now) < 0)
        {
            printf ("[ERROR]"DEBUG_FAMT"\n", DEBUG_ARGS);
            return -1;
        }
    }
    off_t Length = lseek (m_iOpFd, 0, SEEK_END);

    struct tm *pTime = localtime(&now);

    /*如果上次文件是当日打开的且文件没超过最大值*/
    if ((Length < m_uiLogSize) && (m_uiDay == (unsigned int)pTime->tm_yday)) 
    {
        return 0;
    }

    close (m_iOpFd);

    if(m_uiDay == (unsigned int)pTime->tm_yday)
    {
        if(++m_uiSeq > MAX_LOG_CNT)
        {
            printf ("[ERROR]"DEBUG_FAMT"SEQ[%u]\n", DEBUG_ARGS, m_uiSeq);
            return -1;
        }
    }
    else
    {
        m_uiSeq = 0;
    }

    return OpenFd(now);
}

/*******************************************************************************
* Description: 写流水日志
*       Input: pWaterLog:需要填写的字符串地址
*               uiLogLen:字符串长度
*      Output: 无
*      Return: 无
*      Others: 无
*******************************************************************************/
void CTsWaterLog::WriteWaterLog (const char *pWaterLog, unsigned  int uiLogLen)
{  
    time_t now;

    now = time (NULL); 

    //pthread_mutex_lock(&g_MutexLog);
    
    if (ShiftFd(now) < 0)
    {
        //pthread_mutex_unlock(&g_MutexLog);
        printf ("[ERROR]"DEBUG_FAMT"\n", DEBUG_ARGS);
        return ;
    }

    write(m_iOpFd, pWaterLog, uiLogLen);

    //pthread_mutex_unlock(&g_MutexLog);

    return;
}

