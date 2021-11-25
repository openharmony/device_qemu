/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SOC_RANDOM_H__
#define __SOC_RANDOM_H__

/*
 * When kernel decoupled with specific devices,
 * these code can be removed.
 */
VOID HiRandomHwInit(VOID);
VOID HiRandomHwDeinit(VOID);
INT32 HiRandomHwGetInteger(UINT32 *result);
INT32 HiRandomHwGetNumber(CHAR *buffer, size_t bufLen);

#ifdef LOSCFG_HW_RANDOM_ENABLE
void VirtrngInit(void);
#endif

#endif
