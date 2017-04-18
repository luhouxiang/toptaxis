/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称:  toptaxis.cpp
 *       文件功能:  系统入口，实现以下功能
                1: 将程序设置为后台守护进程
                2: 屏蔽相关信号
                3: 创建并监控主工作进程
 *       文件作者:   
 *       修改时间:  2011.03.20 
 *       修改记录:  
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

/*进程锁路径*/
char g_szLockPath[255];

char g_szProcessName[255];

char g_szProcessInfo[256];

int g_iLockFd;

time_t g_tCoredumpNow;
struct tm g_tmCoredump;

/*************************************************************
*函数名称: UniqLock
*输入参数: 略
*返回参数: 略
*函数功能: 互斥锁，只允许打开该程序的一个实例
************************************************************/
int UniqLock()
{
    /*创建以该进程命名的文件标识*/
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
*函数名称: SigtermHandler (int signo)
*输入参数: 略
*返回参数: 略
*函数功能: 退出设置参数为EXIT_MANUAL， 由SIGTERM ,SIGUSR1,SIGUSR2 触发
************************************************************/
void SigtermHandler(int signo) 
{
    int iStatus;

    /*等待子进程退出*/
    wait(&iStatus);


    fprintf( stderr, "father exits success!\n" );
    exit(E_EXIT_MANUAL);
}

/*************************************************************
*函数名称: SigtermHandler1 (int signo)
*输入参数: 略
*返回参数: 略
*函数功能: 退出设置参数为EXIT_MANUAL， 由SIGTERM ,SIGUSR1,SIGUSR2 触发
************************************************************/
void SigtermHandler1(int signo) 
{
    fprintf( stderr, "child exits success!\n" );
    exit(E_EXIT_MANUAL);
}


/*************************************************************
*函数名称: InitDaemon()
*输入参数: 略
*返回参数: 略
*函数功能: 将进程修改为守护进程，并设置信号
************************************************************/
int InitDaemon()
{
    /* 忽略 来自终端的关闭终端信号*/
    signal(SIGHUP,  SIG_IGN); 

    /*忽略 来自终端的终止(interrupt)信号(ctrl+c  )*/
    signal(SIGINT,  SIG_IGN);  

    /*忽略 来自终端的退出(QUIT)信号(ctrl+\ )*/
    signal(SIGQUIT, SIG_IGN); 

    /*忽略操作关闭后的socket而引发的信号*/
    signal(SIGPIPE, SIG_IGN); 


    /*收到进程退出信号时，进行进程退出*/
    signal(SIGTERM, SigtermHandler); 
    signal(SIGUSR1, SigtermHandler); 
    signal(SIGUSR2, SigtermHandler); 


    /*修改为后台进程，工作目录和标准io 保持原样*/
    daemon (1, 1);

    return 0;
}

/*************************************************************
*函数名称: ShowVersion (int argc, char *argv[])
*输入参数: 略
*返回参数: 略
*函数功能: 查询版本号
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
*函数名称: main (int argc, char *argv[])
*输入参数: 略
*返回参数: 略
*函数功能: 系统入口
************************************************************/
int main (int argc, char **argv)
{
    char str_warnmsg[MAX_WARN_LEN] = {0};
    /*获取进程锁文件路径*/
    memset(g_szLockPath, 0, 255);
    memset(g_szProcessName, 0, 255);
    snprintf(g_szProcessName, 254, "%s", argv[0]+2);	
    snprintf(g_szLockPath, 254, "./lock_%s", argv[0]+2);

    ShowVersion(argc, argv);

    /*检查进程是否存在*/
    if(UniqLock())
    {
        printf("\nServer is already Running!\n");
        return E_EXIT_MANUAL;
    }

    /*作为后台进程运行,同时进行信号处理设置*/
    InitDaemon();

    /*创建主工作进程*/
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
        /*收到进程退出信号时，进行进程退出*/
        signal(SIGTERM, SigtermHandler1); 
        signal(SIGUSR1, SigtermHandler1); 
        signal(SIGUSR2, SigtermHandler1); 

        /*初始化工作进程*/
        InitApp();

        exit(E_EXIT_NORMAL);
    }
    else if(iPID > 0)     
    {
        /*等待子进程退出*/
        wait(&iStatus);

        g_tCoredumpNow = time(NULL);
        localtime_r(&g_tCoredumpNow, &g_tmCoredump);

        /*进程退出，需要打印日志，方便二级网管进行监控*/
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
            /*如果子进程人工退出，则不再创建子进程，父进程正常退出*/
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
            /*休眠2秒，等待进程释放相关资源*/
            sleep(2);
            /*重新创建子进程*/
            goto CHILD_POINT;
        }
    }
    return E_EXIT_NORMAL;
    
}


