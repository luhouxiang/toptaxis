/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

int main(int argc, char **argv)
{
    log_init("./log/", 6, 10240000, "test_");
	ERROR_LOG("Write ERROR_LOG test.");
	CRIT_LOG("Write CRIT_LOG test.");
	ALERT_LOG("Write ALERT_LOG test.");
	EMERG_LOG("Write EMERG_LOG test.");
	
	WARN_LOG("Write WARN_LOG test.");
	NOTI_LOG("Write NOTI_LOG test.");
	INFO_LOG("Write INFO_LOG test.");
	
	DEBUG_LOG("Write DEBUG_LOG test.");
    return 0;
}

