/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

#define MAX_BACHE_KEY_NUM           100     //批量处理的最大号码个数
#define MAX_IP_ADDR_LEN             16      //ip 地址的长度
#define LISTEN_PROTOCOL_LEN         16      //通讯协议的长度 ，如tcp,udp
#define MAX_SERVICE_NAME_LEN        512      //名字的最大长度
#define MAX_CACHE_NUM               128     //最大的cache 块个数
#define VIRTUAL_NODE_NUM            540     //虚拟节点的个数
#define MAX_FILE_PATH_LEN           256     //文件路径的最大长度
#define MAX_ERROR_INFO_LEN          256     //错误消息的最大长度
#define MAX_REQUEST_MSG_LEN         32768   //请求消息的最大长度
#define MAX_RESPONSE_MSG_LEN        4096    //响应消息的最大长度
#define BITMAP_HEADER_LEN           11      //bitmap 的头部长度
#define TAXIS_NOTIFY_MSG_LEN        15      //toptaxis与notify同步消息的长度
#define TAXIS_NOTIFY_MSG_BODY_LEN   12      //toptaxis与notify同步消息包体的长度
#define BITMAP_MAX_BODY_LEN         2036    //bitmap 的报文体的最大长度 MAX_REQUEST_MSG_LEN - BITMAP_HEADER_LEN
#define MAX_WARN_LEN                128     //告警描述的最大长度
#define MAX_BNAME_LEN               16      //业务信息表中的业务名称
#define MAX_TNAME_LEN               32      //数据库表相关信息名称
#define MAX_UNSIGNED_SHORT          65535   //		
#define INDEX_MODE_INDEX_LEN        12      //索引模式下索引+key所占空间(Byte)
#define MAX_STORE_DATA_LEN          120    /*单条记录的最大长度为120个字节*/
#define TAXIS_NOTIFY_SEMP_KEY       9999   //toptaxis与notify共享内存的SEMP
#define TAXIS_NOTIFY_SOCKT_PORT     30001 //notify监听消息的端口

#define DATA_SHM_SIZE               512    /*存储中单块内存cache 块的大小512M*/
#define WRITE_CONTROL_CACHE_SIZE    524288 /*write 进程控制cache块的大小 =  512K*/
#define LOGSRV_CONTROL_CACHE_SIZE   1024 /*log server 进程控制cache块的大小 =  1k*/

enum ERROR_CODE_DEFINE
{
    SUCCESS_CODE = 0,
    ERROR_SYSTEM_FAILED                  = -1000,   //系统错误，如指针为空，申请内存失败等
    ERROR_OTHER_FAILED                   = -1001,   //其它错误
    ERROR_REQUEST_MSG_ILLEGAL            = -1002,   //请求消息非法
    ERROR_REQUEST_CMD_WRONG              = -1003,   //请求命令字错误

    /*betwen transfer and store*/
    ERROR_DATA_LEN_EXCEED                = -2001,  //数据记录过长，超过了系统分配的长度
    ERROR_DATA_HAS_EXIT                  = -2002,  //记录存在
    ERROR_DATA_NOT_EXIT                  = -2003,  //记录不存在
    ERROR_BACH_NUM_EXCEED                = -2004,  //一次批量操作号码过多
    ERROR_DATA_HAS_LOCKED                = -2005,  //记录被锁住，正在进行其它操作
    ERROR_ROUTE_IS_WRONG                 = -2006,  //路由信息错误，需要重新拉取路由
    ERROR_NOTIFY_OFFSET_WRONG            = -2007,  //同步偏移量错误，需要重新矫正偏移量
    ERROR_NOTIFY_OFFSET_DONE             = -2008,  //同步偏移量被处理过，存储不需要更新内存
    ERROR_MACHIN_ATTR_WRONG              = -2009,  //机器属性错误
    ERROR_DEST_PORT_IS_WRONG             = -2010,  //对端端口错误
    ERROR_NO_FREE_SPACE                  = -2011,  //资源不够
    ERROR_DONT_SUPPORT_DELETE            = -2013,  //全量数据不支持删除操作

    /*betwen transfer and control*/
    ERROR_ADD_NOTIFY__HAS_EXIT           = -3001,   //增加的notify对端已经存在
    ERROR_DELETE_FILE_FAIL               = -3002,   //删除文件失败
    ERROR_DELETE_NOTIFY_FAIL             = -3003,   //删除notify对端失败
    ERROR_MODIFY_NOTIFY_FAIL             = -3004,   //修改notify对端失败

    /*betwen store and control*/
    ERROR_MASH_ATTRIBUTE_ILLEGAL         = -4001,   //机器的属性信息非法，如为初始化状态
    ERROR_CACHE_HAS_USED                 = -4002,   //cache 块已经被使用了
    ERROR_CACHE_NOT_USED                 = -4003,   //cache 块没有被使用
    ERROR_REBUILD_INDEX_FAIL             = -4004,   //重建索引错误
    ERROR_MEMORY_BACK_FAIL               = -4005,   //备份内存失败
    ERROR_MEMORY_IN_BACKING              = -4006,   //备份正在进行中，请稍后
    ERROR_BACK_FILE_ILLEGAL              = -4007,   //备份的文件非法
    ERROR_MEMORY_RESTORE_FAIL            = -4008,   //还原文件到内存失败
    ERROR_MEMORY_IN_RESTOREING           = -4009,   //还原文件正在进行中，请稍候
    ERROR_MEMORY_TRANSFER_FAIL           = -4010,   //迁移业务失败
    ERROR_MEMORY_IN_TRANSFERING          = -4011,   //迁移正在进行中，请稍候
    ERROR_CACHE_NUM_ILLEGAL              = -4012,   //机器的cache块数目非法
    ERROR_CACHE_STATUS_ILLEGAL           = -4013,   //机器的cache块数目非法

    ERROR_MACH_STORE_ILLEGAL             = -4014,   //存储模式或者内存型号非法
    ERROR_SERVICE_NOT_EIXT               = -4015,   //业务不存在
    ERROR_SERVICE_ILLEGAL                = -4016,   //业务不合法
    ERROR_DATA_IS_IN_USE                 = -4017,   //数据还在被使用
    ERROR_MASHINE_NOT_EXIT               = -4018,   //指定机器的数据不存在
    ERROR_ADD_SERVICE_FAILED             = -4019,   //添加业务失败

    /*betwen control and control*/
    ERROR_EXIT_NONFREE_OR_UNRECH_MASHINE = -5000,   //存在非空闲或不可达的机器
    ERROR_EXIT_UNREACHABLE_MASHINE       = -5001,   //存在不可达的机器
    ERROR_EXIT_NONFREE_MASHINE           = -5002,   //存在非空闲的机器
    ERROR_MASHINE_RELATION_IS_WRONG      = -5003,   //机器关系错误
    ERROR_CONTROL_NO_PERMISSION          = -5004,   //没有操作权限

    ERROR_STOREMODE_NOT_CONSISTENT       = -5005,  //组内存储机器的存储模式不一致
    ERROR_FREE_CACHE_NOT_ENOUGH          = -5006,  // 没有足够的空闲cache块
    ERROR_MYSQL_OPERATE_FAIL             = -5007,  // mysql数据库操作失败
    ERROR_MASHINE_ATTRIBUTE_WRONG        = -5008,  // 机器属性错误
    ERROR_MASHINE_STATUS_WRONG           = -5009,
    ERROR_SERVICE_EXCEED_MAX_SIZE        = -5010,  // 业务上线或扩容时，业务总大小超过最大值
    ERROR_MASHINE_NOT_EXIST              = -5011,  // 新增设备时，相关设备IP在设备表中不存在
    ERROR_EXPAN_EXIST_UNCOMPLETE         = -5012,  // 系统新增设备时存在上次新增设备未完成的步骤
    ERROR_DISASTER_EXIST_UNCOMPLETE      = -5013,  // 系统容灾时存在上次容灾未完成的步骤

    ERROR_LOAD_SERVICE_FAIL              = -5014,  // 新增业务时CTRL给存储加载业务信息失败	

    /*between proxy and store*/
    ERROR_PROXY_NO_ROUTE                 = -6001, // 没有路由
    ERROR_PROXY_WRONG_ROUTE              = -6002, // 路由错误
    ERROR_PROXY_SEND                     = -6003, // 到store 发包失败
    ERROR_PROXY_RECV                     = -6004, // 到store 收包失败
    ERROR_PROXY_TIMEOUT                  = -6005, // 到store 接收超时
    ERROR_PROXY_SYS                      = -6100,
};

#endif
