/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

#define MAX_BACHE_KEY_NUM           100     //������������������
#define MAX_IP_ADDR_LEN             16      //ip ��ַ�ĳ���
#define LISTEN_PROTOCOL_LEN         16      //ͨѶЭ��ĳ��� ����tcp,udp
#define MAX_SERVICE_NAME_LEN        512      //���ֵ���󳤶�
#define MAX_CACHE_NUM               128     //����cache �����
#define VIRTUAL_NODE_NUM            540     //����ڵ�ĸ���
#define MAX_FILE_PATH_LEN           256     //�ļ�·������󳤶�
#define MAX_ERROR_INFO_LEN          256     //������Ϣ����󳤶�
#define MAX_REQUEST_MSG_LEN         32768   //������Ϣ����󳤶�
#define MAX_RESPONSE_MSG_LEN        4096    //��Ӧ��Ϣ����󳤶�
#define BITMAP_HEADER_LEN           11      //bitmap ��ͷ������
#define TAXIS_NOTIFY_MSG_LEN        15      //toptaxis��notifyͬ����Ϣ�ĳ���
#define TAXIS_NOTIFY_MSG_BODY_LEN   12      //toptaxis��notifyͬ����Ϣ����ĳ���
#define BITMAP_MAX_BODY_LEN         2036    //bitmap �ı��������󳤶� MAX_REQUEST_MSG_LEN - BITMAP_HEADER_LEN
#define MAX_WARN_LEN                128     //�澯��������󳤶�
#define MAX_BNAME_LEN               16      //ҵ����Ϣ���е�ҵ������
#define MAX_TNAME_LEN               32      //���ݿ�������Ϣ����
#define MAX_UNSIGNED_SHORT          65535   //		
#define INDEX_MODE_INDEX_LEN        12      //����ģʽ������+key��ռ�ռ�(Byte)
#define MAX_STORE_DATA_LEN          120    /*������¼����󳤶�Ϊ120���ֽ�*/
#define TAXIS_NOTIFY_SEMP_KEY       9999   //toptaxis��notify�����ڴ��SEMP
#define TAXIS_NOTIFY_SOCKT_PORT     30001 //notify������Ϣ�Ķ˿�

#define DATA_SHM_SIZE               512    /*�洢�е����ڴ�cache ��Ĵ�С512M*/
#define WRITE_CONTROL_CACHE_SIZE    524288 /*write ���̿���cache��Ĵ�С =  512K*/
#define LOGSRV_CONTROL_CACHE_SIZE   1024 /*log server ���̿���cache��Ĵ�С =  1k*/

enum ERROR_CODE_DEFINE
{
    SUCCESS_CODE = 0,
    ERROR_SYSTEM_FAILED                  = -1000,   //ϵͳ������ָ��Ϊ�գ������ڴ�ʧ�ܵ�
    ERROR_OTHER_FAILED                   = -1001,   //��������
    ERROR_REQUEST_MSG_ILLEGAL            = -1002,   //������Ϣ�Ƿ�
    ERROR_REQUEST_CMD_WRONG              = -1003,   //���������ִ���

    /*betwen transfer and store*/
    ERROR_DATA_LEN_EXCEED                = -2001,  //���ݼ�¼������������ϵͳ����ĳ���
    ERROR_DATA_HAS_EXIT                  = -2002,  //��¼����
    ERROR_DATA_NOT_EXIT                  = -2003,  //��¼������
    ERROR_BACH_NUM_EXCEED                = -2004,  //һ�����������������
    ERROR_DATA_HAS_LOCKED                = -2005,  //��¼����ס�����ڽ�����������
    ERROR_ROUTE_IS_WRONG                 = -2006,  //·����Ϣ������Ҫ������ȡ·��
    ERROR_NOTIFY_OFFSET_WRONG            = -2007,  //ͬ��ƫ����������Ҫ���½���ƫ����
    ERROR_NOTIFY_OFFSET_DONE             = -2008,  //ͬ��ƫ��������������洢����Ҫ�����ڴ�
    ERROR_MACHIN_ATTR_WRONG              = -2009,  //�������Դ���
    ERROR_DEST_PORT_IS_WRONG             = -2010,  //�Զ˶˿ڴ���
    ERROR_NO_FREE_SPACE                  = -2011,  //��Դ����
    ERROR_DONT_SUPPORT_DELETE            = -2013,  //ȫ�����ݲ�֧��ɾ������

    /*betwen transfer and control*/
    ERROR_ADD_NOTIFY__HAS_EXIT           = -3001,   //���ӵ�notify�Զ��Ѿ�����
    ERROR_DELETE_FILE_FAIL               = -3002,   //ɾ���ļ�ʧ��
    ERROR_DELETE_NOTIFY_FAIL             = -3003,   //ɾ��notify�Զ�ʧ��
    ERROR_MODIFY_NOTIFY_FAIL             = -3004,   //�޸�notify�Զ�ʧ��

    /*betwen store and control*/
    ERROR_MASH_ATTRIBUTE_ILLEGAL         = -4001,   //������������Ϣ�Ƿ�����Ϊ��ʼ��״̬
    ERROR_CACHE_HAS_USED                 = -4002,   //cache ���Ѿ���ʹ����
    ERROR_CACHE_NOT_USED                 = -4003,   //cache ��û�б�ʹ��
    ERROR_REBUILD_INDEX_FAIL             = -4004,   //�ؽ���������
    ERROR_MEMORY_BACK_FAIL               = -4005,   //�����ڴ�ʧ��
    ERROR_MEMORY_IN_BACKING              = -4006,   //�������ڽ����У����Ժ�
    ERROR_BACK_FILE_ILLEGAL              = -4007,   //���ݵ��ļ��Ƿ�
    ERROR_MEMORY_RESTORE_FAIL            = -4008,   //��ԭ�ļ����ڴ�ʧ��
    ERROR_MEMORY_IN_RESTOREING           = -4009,   //��ԭ�ļ����ڽ����У����Ժ�
    ERROR_MEMORY_TRANSFER_FAIL           = -4010,   //Ǩ��ҵ��ʧ��
    ERROR_MEMORY_IN_TRANSFERING          = -4011,   //Ǩ�����ڽ����У����Ժ�
    ERROR_CACHE_NUM_ILLEGAL              = -4012,   //������cache����Ŀ�Ƿ�
    ERROR_CACHE_STATUS_ILLEGAL           = -4013,   //������cache����Ŀ�Ƿ�

    ERROR_MACH_STORE_ILLEGAL             = -4014,   //�洢ģʽ�����ڴ��ͺŷǷ�
    ERROR_SERVICE_NOT_EIXT               = -4015,   //ҵ�񲻴���
    ERROR_SERVICE_ILLEGAL                = -4016,   //ҵ�񲻺Ϸ�
    ERROR_DATA_IS_IN_USE                 = -4017,   //���ݻ��ڱ�ʹ��
    ERROR_MASHINE_NOT_EXIT               = -4018,   //ָ�����������ݲ�����
    ERROR_ADD_SERVICE_FAILED             = -4019,   //���ҵ��ʧ��

    /*betwen control and control*/
    ERROR_EXIT_NONFREE_OR_UNRECH_MASHINE = -5000,   //���ڷǿ��л򲻿ɴ�Ļ���
    ERROR_EXIT_UNREACHABLE_MASHINE       = -5001,   //���ڲ��ɴ�Ļ���
    ERROR_EXIT_NONFREE_MASHINE           = -5002,   //���ڷǿ��еĻ���
    ERROR_MASHINE_RELATION_IS_WRONG      = -5003,   //������ϵ����
    ERROR_CONTROL_NO_PERMISSION          = -5004,   //û�в���Ȩ��

    ERROR_STOREMODE_NOT_CONSISTENT       = -5005,  //���ڴ洢�����Ĵ洢ģʽ��һ��
    ERROR_FREE_CACHE_NOT_ENOUGH          = -5006,  // û���㹻�Ŀ���cache��
    ERROR_MYSQL_OPERATE_FAIL             = -5007,  // mysql���ݿ����ʧ��
    ERROR_MASHINE_ATTRIBUTE_WRONG        = -5008,  // �������Դ���
    ERROR_MASHINE_STATUS_WRONG           = -5009,
    ERROR_SERVICE_EXCEED_MAX_SIZE        = -5010,  // ҵ�����߻�����ʱ��ҵ���ܴ�С�������ֵ
    ERROR_MASHINE_NOT_EXIST              = -5011,  // �����豸ʱ������豸IP���豸���в�����
    ERROR_EXPAN_EXIST_UNCOMPLETE         = -5012,  // ϵͳ�����豸ʱ�����ϴ������豸δ��ɵĲ���
    ERROR_DISASTER_EXIST_UNCOMPLETE      = -5013,  // ϵͳ����ʱ�����ϴ�����δ��ɵĲ���

    ERROR_LOAD_SERVICE_FAIL              = -5014,  // ����ҵ��ʱCTRL���洢����ҵ����Ϣʧ��	

    /*between proxy and store*/
    ERROR_PROXY_NO_ROUTE                 = -6001, // û��·��
    ERROR_PROXY_WRONG_ROUTE              = -6002, // ·�ɴ���
    ERROR_PROXY_SEND                     = -6003, // ��store ����ʧ��
    ERROR_PROXY_RECV                     = -6004, // ��store �հ�ʧ��
    ERROR_PROXY_TIMEOUT                  = -6005, // ��store ���ճ�ʱ
    ERROR_PROXY_SYS                      = -6100,
};

#endif
