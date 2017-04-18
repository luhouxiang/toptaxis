/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*******************************************************************************
*      FileName: vectorExt.cpp
*        Author: 
*     Copyright: Copyright@2011 tencent
*          Date: 2011/9/22
*   Description: 
*       History: 1.   2011/9/22   Ver1.0  build this moudle
*******************************************************************************/
#ifndef  __VECTOR_EXT_H__
#define __VECTOR_EXT_H__

#include <algorithm>
#include <string>
#include <vector>

using namespace std;

/*******************************************************************************
* Description: ��vectorԪ�ؽ�������
*       Input: vec1 �������vector
*              vec2 �������vector
*      Output: ��
*      Return: ��
*      Others: ��
*******************************************************************************/
template<typename T>
void SortVector(vector<T> &vec1, vector<T> &vec2, bool bIsLager)
{
    /*��������*/
    if(bIsLager)
    {
        sort(vec1.begin(), vec1.end(), greater<T>());
        sort(vec2.begin(), vec2.end(), greater<T>());
        return ;
    }

    /*��������*/
    sort(vec1.begin(), vec1.end(), less<T>());
    sort(vec2.begin(), vec2.end(), less<T>());
    return ;
}

/*******************************************************************************
* Description: �Ƚ�����vector�Ƿ���ȫ��ȣ�˳���Ԫ�ض����
*       Input: vec1 ��Ƚϵ�vector
*              vec2 ��Ƚϵ�vector
*      Output: ��
*      Return: 0 ��� ����0 �����
*      Others: ��
*******************************************************************************/
template<typename T>
bool VectorCmpNoSort(vector<T> &vec1, vector<T>&vec2)
{
    if(vec1.size() != vec2.size())
    {
        return false;
    }
    
    typename vector<T>::iterator vIt1 = vec1.begin();
    typename vector<T>::iterator vIt2 = vec2.begin();
    typename vector<T>::iterator vItEnd1 = vec1.end();  

    while(vIt1 != vItEnd1)
    {
        if(*vIt1++ != *vIt2++)
        {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
* Description: �Ƚ�����vector�Ƿ���ȣ�������˳��
*       Input: vec1 ��Ƚϵ�vector
*              vec2 ��Ƚϵ�vector
*      Output: ��
*      Return: 0 ��� ����0 �����
*      Others: ��
*******************************************************************************/
template<typename T>
bool VectorCmpSort(vector<T> vec1, vector<T> vec2)
{
    SortVector(vec1, vec2, true);

    return VectorCmpNoSort(vec1, vec2);
}

/*******************************************************************************
* Description: �Ƚ�����vector�Ƿ���ȣ�����ָ���Ƿ���˳��Ҳ���
*       Input: vec1 ��Ƚϵ�vector
*              vec2 ��Ƚϵ�vector
*              bNeedSort ture:��Ҫ���� false:����Ҫ����
*      Output: ��
*      Return: 0 ��� ����0 �����
*      Others: ��
*******************************************************************************/
template<typename T>
bool VectorCmp(vector<T> &vec1, vector<T> &vec2, bool bNeedSort)
{
    if(bNeedSort)
    {
        return VectorCmpSort(vec1, vec2);
    }
    return VectorCmpNoSort(vec1, vec2);
}

/*******************************************************************************
* Description: ��vector�е�string����Ԫ��ת��Ϊunsigned
*       Input: vStr Դת����vector
*      Output: vUi  Ŀ��ת��vector
*      Return: ��
*      Others: ��
*******************************************************************************/
void VectorStringToUnsigned(vector<string> &vStr, vector<unsigned int> &vUi)
{   
    vector<string>::iterator vIt = vStr.begin();
    vector<string>::iterator vItEnd = vStr.end();

    vUi.clear();
    while(vIt != vItEnd)
    {
        vUi.push_back(strtoul((*vIt).c_str(), NULL, 0));
        vIt++;
    }
    
    return ;
}

#endif

