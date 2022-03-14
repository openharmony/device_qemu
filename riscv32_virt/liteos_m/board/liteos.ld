 /*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH("riscv")
ENTRY(_start)

RAM_ADDR_START            = 0x80000000;
RAM_ADDR_SIZE             = 128M;
FLASH_LOAD_START          = RAM_ADDR_START;
FLASH_LOAD_SIZE           = RAM_ADDR_SIZE;
EXCEPT_STACK_SIZE         = 2K;
NMI_STACK_SIZE            = 1K;
START_AND_IRQ_STACK_SIZE  = 8K;

MEMORY
{
   ram(rwx) : ORIGIN = RAM_ADDR_START, LENGTH = RAM_ADDR_SIZE
   flash(rwx) : ORIGIN = FLASH_LOAD_START, LENGTH = FLASH_LOAD_SIZE
}
SECTIONS
{
  .text : ALIGN(0x20)
  {
    __text_start = .;
    . = ALIGN(0x10);
    *(.start.text)
    *(.interrupt.*)
    *(.text.*)
    . = ALIGN(0x20);
    __rodata_start = .;
    . = ALIGN(0x10);
    *(.rodata.* .srodata.*)
    . = ALIGN(0x20);
    __text_end = .;
    _hdf_drivers_start = .;
    KEEP(*(.hdf.driver))
    _hdf_drivers_end = .;
  } > ram AT > flash
  .data : ALIGN(0x20)
  {
    __global_pointer$ = . + 0x800;
    __data_load_start = LOADADDR(.data);
    __data_start = .;
    . = ALIGN(0x10);
    *(.data.* .sdata.*)
    . = ALIGN(0x20);
    __data_end = .;
    _edata = .;
  } > ram AT > flash
  __data_load_size = SIZEOF(.data);
  .bss (NOLOAD): ALIGN(0x20)
  {
    __bss_start = .;
    . = ALIGN(0x10);
    *(.kernel.bss*)
    *(.bss.* .sbss.*)
    . = ALIGN(0x20);
    __bss_end = .;
  } > ram
  .stack (NOLOAD): ALIGN(0x40)
  {
    __stack_bootom = .;
    . += EXCEPT_STACK_SIZE;
    __except_stack_top = .;
    . += START_AND_IRQ_STACK_SIZE;
    __start_and_irq_stack_top = .;
  } > ram

  .heap (NOLOAD): ALIGN(0x40)
  {
    __heap_start = .;
  } > ram

  __heap_size = RAM_ADDR_SIZE - (__heap_start - __text_start);
  _end = .;
  end = .;
}
