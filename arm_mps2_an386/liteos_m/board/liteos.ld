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

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Vector_Table)

SVC_IRQ_STACK_SIZE  = 4K;
FLASH_START         = 0x00000000;
FLASH_SIZE          = 3M;
PS_RAM_START        = 0x21000000;
PS_RAM_SIZE         = 16M;

MEMORY
{
   flash(rx) : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE
   ram(rwx) : ORIGIN = PS_RAM_START, LENGTH = PS_RAM_SIZE
}

SECTIONS
{
  .vector : {
      KEEP (*(SORT_NONE(.Vector_Table)))
  } > flash

  .got : {
    *(.got.plt)
    *(.igot.plt)
    *(.got)
    *(.igot)
  } > flash

  .text : {
    . = ALIGN(0x4);
    __text_start = .;
    _stext = .;
    *(.text.*)
    . = ALIGN(0x4);
    _etext = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.rodata1)
  } > ram

  .data : {
    __data_start = . ;
    *(.data .data.* .gnu.linkonce.d.*)
  } > ram

  .bss : ALIGN(0x4) {
   __bss_start = .;
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   . = ALIGN(0x4);
   __bss_end = .;
  } > ram

  _sstack = .;
  .stack : ALIGN(0x8) {
    _stack = .;
    __irq_stack_bootom = .;
    . += SVC_IRQ_STACK_SIZE;
    __irq_stack_top = .;
    _estack = .;
  } > ram

  .heap (NOLOAD): ALIGN(0x40) {
    __heap_start = .;
  } > ram

  __heap_size = PS_RAM_SIZE - (__heap_start - __text_start);
  end = .;
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
