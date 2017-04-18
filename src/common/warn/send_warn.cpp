/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Attr_API.h"
#include "send_warn.h"
#include "xmlParser.h"


#define _get_xml_attr(x,y,z) (NULL == x.getAttribute(y) ? z : x.getAttribute(y)) 
CWarnConf* CWarnConf::m_Instance = NULL;


/*************************************************************
*��������: CWarnConf()
*�������: 
*���ز���:  
*��������: ���캯������ʼ������
************************************************************/
CWarnConf::CWarnConf()
{
	m_iRetryTime=3;
	m_iWarnID=128511;
	m_bFlag=false;
}

/*************************************************************
*��������: ~CWarnConf()
*�������: 
*���ز���:  
*��������: ��������
************************************************************/
CWarnConf::~CWarnConf()
{
	if(NULL != m_Instance)	
	{
		delete m_Instance;
		m_Instance = NULL;
	}       
}

/*************************************************************
*��������: Instance()
*�������: 
*���ز���: ��ʵ��ָ��
*��������: ����һ�����ʵ��   
************************************************************/
CWarnConf* CWarnConf::Instance()
{
	if ( NULL == m_Instance )
	{
		m_Instance = new CWarnConf;
	}
	return m_Instance;
}


/*************************************************************
*��������: ReadCfg()
*�������: 
*���ز���: -1:ʧ�ܣ�0:�ɹ�
*��������: ��ȡ�����ļ�
************************************************************/
int CWarnConf::ReadCfg(const char* pConfigFilePath)
{
	if(NULL == pConfigFilePath)	
	{
		fprintf( stderr, "[ReadCfg] The path of config file is invalid\n" );
		return -1;	   
	}

	/*���������ļ��Ƿ����*/
	if (access (pConfigFilePath, F_OK|R_OK))
	{
		fprintf( stderr, "[ReadCfg] The config file is not exists or not read\n" );
		return -1;
	}

	/*�������ļ����ص�ȫ���ڴ�*/
	XMLNode xMainNode= XMLNode::openFileHelper( pConfigFilePath, "content" ); 
    
	/*��ȡ�澯���ò���*/
	XMLNode  xNode = xMainNode.getChildNode( "warn" );
	if(NULL != xNode.getAttribute("warnid"))
	{
		m_iWarnID= atoi(xNode.getAttribute("warnid"));
	}
	else
	{
		m_iWarnID=128511;
	}

	if(NULL != xNode.getAttribute("retrytime"))
	{
		m_iRetryTime=atoi(xNode.getAttribute("retrytime"));
	}
	else
	{
		m_iRetryTime=3;
	}

	return 0;
}

int CWarnConf::SendWarn(string strWarnMsg)
{
	if(!m_bFlag)
	{
		if(ReadCfg(WARN_CONF_DIR))
		{
			return -1;
		}
		m_bFlag=true;
	}

	int i=0;
	for(;i<m_iRetryTime;++i)
	{			
		if(0==adv_attr_set(m_iWarnID,strWarnMsg.size(),(char*)strWarnMsg.c_str()))
		{
			break;
		}
	}

	if(i==m_iRetryTime)
	{
		return -1;
	}

	return 0;
}



/*************************************************************
*��������: PrintLog()
*�������:
*���ز���: 
*��������: ������Ϣ����ӡ��ȡ����������
************************************************************/
void CWarnConf::PrintLog()
{
	return;
}
