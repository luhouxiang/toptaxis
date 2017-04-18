/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef COMM_SEMP_H
#define COMM_SEMP_H

#include <sys/sem.h>

#define COMM_SEMP_PERM  0600

class CCommSemp
{
public :
    CCommSemp() : m_iSempId( -1 ){}
    ~CCommSemp();
    int Init( unsigned uiSempKey );
    int Lock();
    int Unlock();
private :
    unsigned m_uiSempKey; //key
    int m_iSempId; //semp

    static struct sembuf m_stSempLockFlag; //锁标记变量，避免每次都赋值
    static struct sembuf m_stSempUnlockFlag; //解除锁标记变量
};

#endif

