/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       �ļ�����:  toptaxis.cpp
 *       �ļ�����:  ϵͳ��ڣ�ʵ�����¹���
                1: ����������Ϊ��̨�ػ�����
                2: ��������ź�
                3: �������������������
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.20 
 *       �޸ļ�¼:  
 ***************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include<sys/wait.h>
#include<string.h>
#include <sys/file.h>
#include <time.h>
#include "toptaxis_main.h"
#include "toptaxis_header.h"
#include "send_warn.h"

/*������·��*/
char g_szLockPath[255];

char g_szProcessName[255];

char g_szProcessInfo[256];

int g_iLockFd;

time_t g_tCoredumpNow;
struct tm g_tmCoredump;

/*************************************************************
*��������: UniqLock
*�������: ��
*���ز���: ��
*��������: ��������ֻ����򿪸ó����һ��ʵ��
************************************************************/
int UniqLock()
{
    /*�����Ըý����������ļ���ʶ*/
    g_iLockFd = open(g_szLockPath, O_RDWR|O_CREAT|O_APPEND, 0640);

    if(g_iLockFd < 0 )
    {
        printf("\nOpen Lock File %s Failed,  Server Init Failed!\n",g_szLockPath);
        return -1;
    }

    int iRet = flock(g_iLockFd, LOCK_EX | LOCK_NB);
    if(iRet < 0 )
    {
        return -1;
    }

    return 0;

}


/*************************************************************
*��������: SigtermHandler (int signo)
*�������: ��
*���ز���: ��
*��������: �˳����ò���ΪEXIT_MANUAL�� ��SIGTERM ,SIGUSR1,SIGUSR2 ����
************************************************************/
void SigtermHandler(int signo) 
{
    int iStatus;

    /*�ȴ��ӽ����˳�*/
    wait(&iStatus);


    fprintf( stderr, "father exits success!\n" );
    exit(E_EXIT_MANUAL);
}

/*************************************************************
*��������: SigtermHandler1 (int signo)
*�������: ��
*���ز���: ��
*��������: �˳����ò���ΪEXIT_MANUAL�� ��SIGTERM ,SIGUSR1,SIGUSR2 ����
************************************************************/
void SigtermHandler1(int signo) 
{
    fprintf( stderr, "child exits success!\n" );
    exit(E_EXIT_MANUAL);
}


/*************************************************************
*��������: InitDaemon()
*�������: ��
*���ز���: ��
*��������: �������޸�Ϊ�ػ����̣��������ź�
************************************************************/
int InitDaemon()
{
    /* ���� �����ն˵Ĺر��ն��ź�*/
    signal(SIGHUP,  SIG_IGN); 

    /*���� �����ն˵���ֹ(interrupt)�ź�(ctrl+c  )*/
    signal(SIGINT,  SIG_IGN);  

    /*���� �����ն˵��˳�(QUIT)�ź�(ctrl+\ )*/
    signal(SIGQUIT, SIG_IGN); 

    /*���Բ����رպ��socket���������ź�*/
    signal(SIGPIPE, SIG_IGN); 


    /*�յ������˳��ź�ʱ�����н����˳�*/
    signal(SIGTERM, SigtermHandler); 
    signal(SIGUSR1, SigtermHandler); 
    signal(SIGUSR2, SigtermHandler); 


    /*�޸�Ϊ��̨���̣�����Ŀ¼�ͱ�׼io ����ԭ��*/
    daemon (1, 1);

    return 0;
}

/*************************************************************
*��������: ShowVersion (int argc, char *argv[])
*�������: ��
*���ز���: ��
*��������: ��ѯ�汾��
************************************************************/
int ShowVersion (int argc, char **argv)
{
	if(argc == 2)
	{
		string strTem = argv[1];
		if(strTem == "-v" || strTem == "-V")
		{
			printf("%s \n",BITSERVICE_VERSION);
			exit(E_EXIT_NORMAL);			
		}
	}

    return 0;
}

/*************************************************************
*��������: main (int argc, char *argv[])
*�������: ��
*���ز���: ��
*��������: ϵͳ���
************************************************************/
int main (int argc, char **argv)
{
    char str_warnmsg[MAX_WARN_LEN] = {0};
    /*��ȡ�������ļ�·��*/
    memset(g_szLockPath, 0, 255);
    memset(g_szProcessName, 0, 255);
    snprintf(g_szProcessName, 254, "%s", argv[0]+2);	
    snprintf(g_szLockPath, 254, "./lock_%s", argv[0]+2);

    ShowVersion(argc, argv);

    /*�������Ƿ����*/
    if(UniqLock())
    {
        printf("\nServer is already Running!\n");
        return E_EXIT_MANUAL;
    }

    /*��Ϊ��̨��������,ͬʱ�����źŴ�������*/
    InitDaemon();

    /*��������������*/
    pid_t iPID = 0;

    int iStatus, iRet;

    g_tCoredumpNow = 0;
    bzero(&g_tmCoredump, sizeof(g_tmCoredump));
    CWarnConf::Instance()->ReadCfg(WARN_CONF_DIR);

    CHILD_POINT:

    iPID = fork();
    if(iPID < 0)
    {
        return -1;
    }
    else if(0 == iPID)       
    {
        /*�յ������˳��ź�ʱ�����н����˳�*/
        signal(SIGTERM, SigtermHandler1); 
        signal(SIGUSR1, SigtermHandler1); 
        signal(SIGUSR2, SigtermHandler1); 

        /*��ʼ����������*/
        InitApp();

        exit(E_EXIT_NORMAL);
    }
    else if(iPID > 0)     
    {
        /*�ȴ��ӽ����˳�*/
        wait(&iStatus);

        g_tCoredumpNow = time(NULL);
        localtime_r(&g_tCoredumpNow, &g_tmCoredump);

        /*�����˳�����Ҫ��ӡ��־������������ܽ��м��*/
        snprintf(g_szProcessInfo, sizeof(g_szProcessInfo) - 1, "[%04d-%02d-%02d %02d:%02d:%02d] %s exit ! EXIT[%d] STATUS[%d] SIG[%d] TERMSIG[%d]\n"
        , g_tmCoredump.tm_year + 1900, g_tmCoredump.tm_mon + 1, g_tmCoredump.tm_mday, g_tmCoredump.tm_hour, g_tmCoredump.tm_min, g_tmCoredump.tm_sec, g_szProcessName
        , WIFEXITED( iStatus), WEXITSTATUS(iStatus),WIFSIGNALED(iStatus), WTERMSIG(iStatus));

        iRet = write(g_iLockFd, g_szProcessInfo, strlen(g_szProcessInfo));

        fprintf(stderr,"WIFEXITED( iStatus) %d\n",WIFEXITED( iStatus));
        fprintf(stderr,"WEXITSTATUS( iStatus) %d\n",WEXITSTATUS(iStatus));
        fprintf(stderr,"WIFSIGNALED(iStatus) %d\n",WIFSIGNALED(iStatus));
        fprintf(stderr,"WTERMSIG(iStatus) %d\n", WTERMSIG(iStatus));

        if((WIFEXITED( iStatus))&&(E_EXIT_MANUAL == WEXITSTATUS(iStatus)))
        {
            /*����ӽ����˹��˳������ٴ����ӽ��̣������������˳�*/
            exit(E_EXIT_NORMAL);
        }
        else
        {
            snprintf(str_warnmsg, MAX_WARN_LEN, "toptaxis's process is coredump, restart...");
            if(0 != CWarnConf::Instance()->SendWarn(str_warnmsg))
            {
                fprintf(stderr,"sendwarn fail\n");
            }
            else
            {
                fprintf(stderr,"sendwarn ok\n");
            }
            /*����2�룬�ȴ������ͷ������Դ*/
            sleep(2);
            /*���´����ӽ���*/
            goto CHILD_POINT;
        }
    }
    return E_EXIT_NORMAL;
    
}


