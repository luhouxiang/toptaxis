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
*   Description: TSϵͳд��ˮ�ļ�
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

/*���߳�д��ˮ��־�߳���*/
pthread_mutex_t g_MutexLog;

CTsWaterLog * CTsWaterLog::m_pInstance = NULL;

/*******************************************************************************
* Description: ���캯��
*       Input: ��               
*      Output: ��
*      Return: ��
*      Others: ��
*******************************************************************************/
CTsWaterLog::CTsWaterLog()
{
    m_iOpFd = -1;   
    memset(m_szLogPreName, 0, sizeof(m_szLogPreName));
}

/*******************************************************************************
* Description: ��������
*       Input: ��               
*      Output: ��
*      Return: ��
*      Others: ��
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
* Description: ��ȡ���
*       Input: ��               
*      Output: ��
*      Return: ���
*      Others: ��
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
* Description: ���յ�ǰ�����ں�SEQ�����ļ���
*       Input: uiSeqArg:seq      
*         now: ��ǰʱ��
*      Output: pFileName: ���ɵ��ļ���
*      Return: ��
*      Others: ��
*******************************************************************************/
void CTsWaterLog::LogFileName(unsigned int uiSeqArg, char* pFileName, time_t now)
{
    struct tm stTime;

    localtime_r(&now, &stTime);

    snprintf (pFileName, MAX_FILENAME_LEN - 1, "%s/%s%04d%02d%02d%03d", m_szLogDir, m_szLogPreName, 
    stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday, uiSeqArg);
}

/*******************************************************************************
* Description: ��ȡ��ˮ��־�������µ���־SEQ,û����־��Ϊ0
*       Input: ��
*      Output: ��
*      Return: ����������־��SEQ
*      Others: ��
*******************************************************************************/
unsigned int CTsWaterLog::GetLogSeq()
{
    char szFileName[MAX_FILENAME_LEN];

    time_t now = time(NULL);
    
    unsigned int uiSeqTmp;
    for(uiSeqTmp = 0; uiSeqTmp < MAX_LOG_CNT; uiSeqTmp++)
    {
        /*���յ�ǰ���ں�SEQ�����ļ���*/
        LogFileName(uiSeqTmp, szFileName, now);

        /*�жϸ��ļ��Ƿ����*/
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

    /*������־����������*/
    if (MAX_LOG_CNT == uiSeqTmp) 
    {
        printf ("[ERROR]"DEBUG_FAMT"\n", DEBUG_ARGS);
        return -1;
    }
    
    /*���ز����ڵ�SEQ����һ��SEQ��������û����ˮ��־�򷵻�0*/
    return uiSeqTmp == 0 ? uiSeqTmp : uiSeqTmp - 1;    
}

/*******************************************************************************
* Description: ��ʼ����ˮ��־��
*       Input: pLogDir:��־Ŀ¼
*          pLogPreName:��־ǰ׺
*            uiLogSize:��־��¼���ֵ 
*      Output: ��
*      Return: 0/-1
*      Others: ��
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
* Description: �򿪵������µ���־
*       Input: now:����ʱ��
*      Output: m_iOpFd:��־�ļ�FD
*      Return: m_iOpFd:��־�ļ�FD
*      Others: ��
*******************************************************************************/
int CTsWaterLog::OpenFd(time_t now)
{
    char szFileName[MAX_FILENAME_LEN];

    /*���յ������µ�SEQ�����ļ���*/
    LogFileName (m_uiSeq, szFileName, now);

    /*��ȡ�ļ����������������򴴽��ļ����ļ��򿪺���Ҫ��¼�ļ�������*/
    m_iOpFd = open (szFileName, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if(m_iOpFd < 0)
    {
        printf ("[ERROR]"DEBUG_FAMT"OPFD[%d]\n", DEBUG_ARGS, m_iOpFd);
        return m_iOpFd;
    }

    /*��¼�ļ���ʱ��*/
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
* Description: �ж��ļ���С�Ƿ񳬹����ֵ�����������ֵ�����½��ļ�
*       Input: now:����ʱ��
*      Output: m_uiSeq:��־�ļ�SEQ
*      Return: m_iOpFd:��־�ļ�FD
*      Others: ��
*******************************************************************************/
int CTsWaterLog::ShiftFd (time_t now)
{    
    /*��ǰδ�򿪹���־�ļ���������µ��ļ������ļ��������򴴽��ļ�*/
    if (m_iOpFd < 0) 
    {
        /*��ȡ������־�ļ����µ�SQE���������ļ���Ϊ0*/
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

    /*����ϴ��ļ��ǵ��մ򿪵����ļ�û�������ֵ*/
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
* Description: д��ˮ��־
*       Input: pWaterLog:��Ҫ��д���ַ�����ַ
*               uiLogLen:�ַ�������
*      Output: ��
*      Return: ��
*      Others: ��
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

