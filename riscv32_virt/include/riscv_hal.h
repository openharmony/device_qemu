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

#ifndef _RISCV_HAL_H
#define _RISCV_HAL_H

#include "los_compiler.h"
#include "los_context.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*
 * backtrace
 */
extern CHAR *__except_stack_top;
extern CHAR *__start_and_irq_stack_top;
extern CHAR *__text_start;
extern CHAR *__text_end;
extern CHAR *__bss_end;

extern VOID HalIrqDisable(UINT32 vector);
extern VOID HalIrqEnable(UINT32 vector);
extern VOID HalSetLocalInterPri(UINT32 vector, UINT16 prior);

extern VOID HalGetSysCpuCycle(UINT32 *cntHi, UINT32 *cntLo);
extern VOID HalClockInit(OS_TICK_HANDLER handler, UINT32 period);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _RISCV_HAL_H */
