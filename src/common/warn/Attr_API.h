/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef ATTR_API_H
#define ATTR_API_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * ���Ź����ڴ�д��ֵ������ֵ
 * ���ָ�������ѱ�����ֵ,��ֵ����ԭֵ
 * attr: ����ID
 * iValue: ����ֵ
 * return: -1��ʾʧ��;0��ʾ�ɹ�
 */
int Attr_API_Set(int attr,int iValue);

/**
 * ��ȡ��ֵ�����Ե�ֵ
 * ���ָ�������ѱ�����ֵ,��ֵ����ԭֵ
 * attr: ����ID
 * iValue: ����ֵ�洢��ַ
 * return: -1��ʾʧ��;0��ʾ�ɹ� 
 */
int Get_Attr_Value(int attr,int *iValue); 

/**
 * ���Ƽ�ʹ��
 * ���Ź����ڴ�д��ֵ������ֵ
 * ���ָ�������ѱ�����ֵ,��ֵ���ۼӵ�ԭֵ��
 * attr: ����ID
 * iValue: �ۼ�ֵ
 */
int Attr_API(int attr,int iValue);

/**
 * ȡδ�ò��Ź����ڴ��С
 * ����ֵ��1��ʾʧ��;������ʾδ�ù����ڴ��С
 */ 
int get_adv_memlen();

/**
 * ȡ���ò��Ź����ڴ��С
 * ����ֵ��1��ʾʧ��;������ʾ�����ù����ڴ��С
 */  
int get_adv_memusedlen();

/**
 * ���Ź����ڴ�д����,��agent���͸����ܷ���������
 * attr_id:���ݵ�����id����600��ʼ��С��600Ϊ�Ƿ�
 * len���ݳ��ȣ���ҪС�ڹ����ڴ�Ŀ��ô�С�������ڴ��ʼ���ô�С��2Mk �� sizeof(int)
 * pvalue:����ʵ��ҵ�����ݣ��ǿա�
 * ����ֵ0��ʾ�ɹ�������ʧ��
 * ����ע�⣺����������ε������ò�ͬ���ݣ����ݽ��������������У�ֱ��2M
 */ 
int adv_attr_set(int attr_id , size_t len , char* pvalue);

/**
 * ȡ���Ź����ڴ������,ע��pOut�����߷��䣬�Ҵ�Сһ��Ҫ���ڻ����len
 * offset:ƫ��������ʾ�Ӳ��Ź����ڴ濪ʼoffset���ȿ�ʼȡֵ
 * len��ȡ�����ݵĳ��ȣ�������ڹ����ڴ��С��ȡ�����ڴ���󳤶�
 * pOut:�������buffer���ɵ����߷��䣬ע���Сһ��Ҫ���ڻ����len
 */ 
int get_adv_mem(size_t offset , size_t len , char* pOut);

/**
 * ��IP�ϱ���ֵ��ҵ��������ֵ
 * strIP: �ַ���IP��ַ
 * iAttrID: ����id
 * iValue: ����ֵ
 * �ɹ�����0��ʧ�ܷ���-1
 */ 
int setNumAttrWithIP(const char* strIP, int iAttrID, int iValue);

/**
 * ��IP�ϱ��ֽ���ҵ������
 * strIP: �ַ���IP��ַ
 * iAttrID: ����id
 * len: �ֽڴ����Գ���
 * pval: �ֽڴ������׵�ַ
 * �ɹ�����0��ʧ�ܷ���-1
*/
int setStrAttrWithIP(const char* strIP, int iAttrID, size_t len , char* pval);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // ATTR_API_H

