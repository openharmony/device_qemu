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

#include "plic.h"
#include "soc.h"
#include "los_reg.h"
#include "los_arch_interrupt.h"
#include "los_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

STATIC VOID OsMachineExternalInterrupt(VOID *arg)
{
    volatile UINT32 *plicReg = (volatile UINT32 *)(PLIC_REG_BASE + 0x4);
    UINT32 irqNum, saveIrqNum;

    READ_UINT32(irqNum, plicReg);
    saveIrqNum = irqNum;

    if ((irqNum >= OS_RISCV_CUSTOM_IRQ_VECTOR_CNT) || (irqNum == 0)) {
        HalHwiDefaultHandler((VOID *)irqNum);
    }

    irqNum += RISCV_SYS_MAX_IRQ;

    g_hwiForm[irqNum].pfnHook(g_hwiForm[irqNum].uwParam);

    WRITE_UINT32(saveIrqNum, plicReg);
}

#define OS_PLIC_MAX ((OS_RISCV_CUSTOM_IRQ_VECTOR_CNT >> 5) + 1)

VOID PlicIrqInit()
{
    volatile UINT32 *plicPrioReg = (volatile UINT32 *)PLIC_PRIO_BASE;
    volatile UINT32 *plicEnReg = (volatile UINT32 *)PLIC_ENABLE_BASE;
    volatile UINT32 *plicReg = (volatile UINT32 *)PLIC_REG_BASE;
    INT32 i;
    UINT32 ret;

    for (i = 0; i < OS_PLIC_MAX; i++) {
        WRITE_UINT32(0x0, plicEnReg);
        plicEnReg++;
    }

    for (i = 0; i < OS_RISCV_CUSTOM_IRQ_VECTOR_CNT; i++) {
        WRITE_UINT32(0x0, plicPrioReg);
        plicPrioReg++;
    }

    WRITE_UINT32(0, plicReg);

    ret = LOS_HwiCreate(RISCV_MACH_EXT_IRQ, 0x1, 0, OsMachineExternalInterrupt, 0);
    if (ret != LOS_OK) {
        PRINT_ERR("Creat machine external failed! ret : 0x%x\n", ret);
    }
}

VOID PlicIrqSetPrio(UINT32 vector, UINT32 pri)
{
    volatile UINT32 *plicReg = (volatile UINT32 *)PLIC_PRIO_BASE;

    plicReg += (vector - RISCV_SYS_MAX_IRQ);
    WRITE_UINT32(pri, plicReg);
}

VOID PlicIrqEnable(UINT32 vector)
{
    UINT32 irqValue;
    UINT32 locIrq = vector - RISCV_SYS_MAX_IRQ;
    volatile UINT32 *plicReg = (volatile UINT32 *)PLIC_ENABLE_BASE;

    plicReg += (locIrq >> 5); /* 5： The PLIC interrupt controls the bit width */
    READ_UINT32(irqValue, plicReg);
    irqValue |= (1 << (locIrq & 31)); /* 31: plic irq mask */
    WRITE_UINT32(irqValue, plicReg);
}

VOID PlicIrqDisable(UINT32 vector)
{
    UINT32 irqValue;
    UINT32 locIrq = vector - RISCV_SYS_MAX_IRQ;
    volatile UINT32 *plicReg = (volatile UINT32 *)PLIC_ENABLE_BASE;

    plicReg += (locIrq >> 5); /* 5： The PLIC interrupt controls the bit width */
    READ_UINT32(irqValue, plicReg);
    irqValue &= ~(1 << (locIrq & 31)); /* 31: plic irq mask */
    WRITE_UINT32(irqValue, plicReg);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

