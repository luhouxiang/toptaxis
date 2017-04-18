/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef __TIMERLIST_H__
#define __TIMERLIST_H__

#include "list.h"
#include "timestamp.h"

class CTimerObject;
class CTimerUnit;

class CTimerList {
private:
	CListObject<CTimerObject> tlist;
	int timeout;
	CTimerList *next;

public:
	friend class CTimerUnit;
	friend class CTimerObject;
	CTimerList(int t) : timeout(t), next(NULL) { }
	~CTimerList(void) { tlist.FreeList(); }
	int CheckExpired(int64_t now=0);
};

class CTimerUnit {
private:
	CTimerList pending;
	CTimerList *next;
public:
	friend class CTimerObject;
	CTimerUnit(void);
	~CTimerUnit(void);

	CTimerList *GetTimerListByMSeconds(int);
	CTimerList *GetTimerList(int t) {return GetTimerListByMSeconds(t*1000);}
	int ExpireMicroSeconds(int);
	int CheckExpired(int64_t now=0);
	int CheckReady(void);
};

class CTimerObject: private CListObject<CTimerObject> {
private:
	int64_t objexp;

public:
	friend class CTimerList;
	friend class CTimerUnit;
	CTimerObject() { }
	virtual ~CTimerObject(void);
	virtual void TimerNotify(void);
	void DisableTimer(void) { ResetList(); }
	void AttachTimer(class CTimerList *o);
	void AttachReadyTimer(class CTimerUnit *o) { ListMoveTail(o->pending.tlist); }
};

template<class T>
class CTimerMember: public CTimerObject
{
private:
	T *owner;
	virtual void TimerNotify(void) { owner->TimerNotify(); }
public:
	CTimerMember(T *o) : owner(o) {}
	virtual ~CTimerMember() {}
};

#endif
