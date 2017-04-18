/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include "comm_semp.h"
#include "log.h"


struct sembuf CCommSemp::m_stSempLockFlag = { 0, -1, SEM_UNDO };
struct sembuf CCommSemp::m_stSempUnlockFlag = { 0, 1, SEM_UNDO };

CCommSemp::~CCommSemp()
{
    //删除已创建的semaphore
    if ( -1 != m_iSempId )
    {
        //semctl( m_iSempId, 0, IPC_RMID );
    }    
}

int CCommSemp::Init( unsigned uiSempKey )
{
    int iRet = 0;
    m_uiSempKey = uiSempKey;

    //根据key获取对应的semaphore
    m_iSempId = semget( uiSempKey, 0, 0 );
    if ( -1 == m_iSempId ) //semaphore还没有创建，需要创建
    {
        m_iSempId = semget( uiSempKey, 1, IPC_CREAT | COMM_SEMP_PERM );
        if ( -1 != m_iSempId ) //创建新semaphore成功，进行初始化
        {
              /*
        struct sembuf stSempBuf;
        stSempBuf.sem_num = 0;
        stSempBuf.sem_op = 1;  
        stSempBuf.sem_flg = 0;
        iRet = semop( m_iSempId, &stSempBuf, 1 );
        */
            //初始化信号量值为1
            iRet = semctl( m_iSempId, 0, SETVAL, 1 );
            if ( -1 == iRet ) //初始化失败，抛异常??
            {
                ERROR_LOG( "Init new semaphore fail[%d][%s]", errno, strerror(errno) );
                return -1;
            }
        }
        else if ( EEXIST == errno ) //创建前已经有另一个进程或者线程创建了这个semaphore
        {
            m_iSempId = semget( uiSempKey, 0, 0 );
            if ( -1 == m_iSempId )
            {
                ERROR_LOG( "semget old semaphore fail[%d][%s]", errno, strerror(errno) );
                return -1;
            }
        }
        else
        {
            ERROR_LOG( "create new semaphore fail[%d][%s]", errno, strerror(errno) );
            return -1;
        }
    }

    DEBUG_LOG("sempid[%d]",m_iSempId);

    return 0;
}

int CCommSemp::Lock()
{
    while ( semop( m_iSempId, &m_stSempLockFlag, 1) < 0 )
    {
        if ( errno != EINTR ) 
        {    
            ERROR_LOG( "Lock fail[%d][%s]semid[%d]", errno, strerror(errno),m_iSempId );
            return -1;
        }
    }
    
    return 0;
}

int CCommSemp::Unlock()
{
    while ( semop( m_iSempId, &m_stSempUnlockFlag, 1) < 0 ) 
    {
        if ( errno != EINTR ) 
        {    
            ERROR_LOG( "Unlock fail[%d][%s]semid[%d]", errno, strerror(errno),m_iSempId );
            return -1;
        }
    }

    return 0;
}
