/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     liteos.ld
 * @brief    csky linker file
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
MEMORY
{
  I-SRAM : ORIGIN = 0x0        , LENGTH = 0x20000   /* I-SRAM  128KB */
  D-SRAM : ORIGIN = 0x20000000 , LENGTH = 0x20000   /* D-SRAM  128KB */
  O-SRAM : ORIGIN = 0x50000000 , LENGTH = 0x800000   /* off-chip SRAM 8MB */
  SRAM   : ORIGIN = 0x60000000 , LENGTH = 0x20000   /* on-chip SRAM 128KB */
}

_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

PROVIDE (__ram_end  = ORIGIN(O-SRAM) + LENGTH(O-SRAM));

REGION_ALIAS("REGION_TEXT",    O-SRAM);
REGION_ALIAS("REGION_RODATA",  O-SRAM);
REGION_ALIAS("REGION_DATA",    O-SRAM);
REGION_ALIAS("REGION_BSS",     O-SRAM);

ENTRY(Reset_Handler)
SECTIONS
{
  .text : {
    . = ALIGN(0x4) ;
    KEEP(*startup.o(.text))
    __text_start = .;
    __stext = . ;
    *(.text)
    *(.text*)
    *(.text.*)
    *(.stub)
    *(.gnu.linkonce.t*)
    *(.glue_7t)
    *(.glue_7)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN (4) ;
    __etext = . ;
    __text_end = .;
  } > REGION_TEXT

  .rodata : {
    . = ALIGN(0x4) ;
    __rodata_start = .;
    __srodata = .;
    *(.rdata)
    *(.rdata*)
    *(.rdata.*)
    *(.rodata)
    *(.rodata*)
    *(.rodata.*)
    *(.rodata.str1.4)
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    . = ALIGN(0x4) ;
    __erodata = .;
    __rodata_end = .;
  } > REGION_RODATA

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >REGION_RODATA

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >REGION_RODATA

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >REGION_RODATA

  .data : {
    . = ALIGN(0x4) ;
    __sdata = . ;
    __data_start__ = . ;
    __ram_data_start = __data_start__;
    data_start = . ;
    *(.sdata)
    *(.sdata.*)
    *(.data)
    *(.data*)
    *(.data.*)
    *(__libc_subfreeres)
    *(.note.ABI-tag)
    KEEP(*( SORT (.liteos.table.*)));
    . = ALIGN(0x4) ;
    __edata = .;
    __data_end__ = .;
    __ram_data_end = __data_end__;
  } > REGION_DATA AT > REGION_RODATA

  .eh_frame : ONLY_IF_RW {
    KEEP (*(.eh_frame))
  } > REGION_DATA AT > REGION_RODATA

  .gcc_except_table : ONLY_IF_RW {
    *(.gcc_except_table .gcc_except_table.*)
    __edata = .;
    __data_end__ = .;
  } > REGION_DATA AT > REGION_RODATA

  .bss : {
    . = ALIGN(0x4) ;
    __sbss = ALIGN(0x4) ;
    __bss_start = . ;
    *(.sbss)
    *(.sbss.*)
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __init_stack_s = ABSOLUTE(.);
    . += 0x1000;
    __init_stack_e = ABSOLUTE(.);
    . = ALIGN(0x4) ;
    __ebss = . ;
    __bss_end = .;
    __end = . ;
    end = . ;
  } > REGION_BSS

  . = ALIGN(0x8);
  __heap_start = .;
  __heap_end = ORIGIN(O-SRAM) + LENGTH(O-SRAM) - _Min_Stack_Size - 4;
  __heap_size = __heap_end - __heap_start;
}
