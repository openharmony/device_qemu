/****************************************************************************
 * device/qemu/esp32/ESP32.ld
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

MEMORY {
  drom   (r)    : ORIGIN = 0x3f400010, LENGTH = 0x800000
  extmem (rwx)  : ORIGIN = 0x3f800000, LENGTH = 0x400000
  dram   (rw)   : ORIGIN = 0x3ffb0000, LENGTH = 0x50000
  iram_0  (rx)  : ORIGIN = 0x40080000, LENGTH = 0x20000
  iram_1  (rx)  : ORIGIN = 0x400D0018, LENGTH = 0x330000
}

_eram = ORIGIN(dram) + LENGTH(dram);

/* Default entry point: */
ENTRY(main);

SECTIONS
{
  /* Send .iram0 code to iram */

  .iram0.vectors :
  {
    /* Vectors go to IRAM */

    _init_start = ABSOLUTE(.);

    /* Vectors according to builds/RF-2015.2-win32/esp108_v1_2_s5_512int_2/config.html */
     . = 0x0;
    KEEP(*(.WindowVectors.text));
    . = 0x180;
    KEEP(*(.InterruptEntry2.text));
    . = 0x1c0;
    KEEP(*(.InterruptEntry3.text));
    . = 0x200;
    KEEP(*(.InterruptEntry4.text));
    . = 0x240;
    KEEP(*(.InterruptEntry5.text));
    . = 0x280;
    KEEP(*(.DebugExceptionHandler.text));
    . = 0x2c0;
    KEEP(*(.NMIExceptionHandler.text));
    . = 0x300;
    KEEP(*(.KernelExceptionHandler.text));
    . = 0x340;
    KEEP(*(.UserExceptionHandler.text));
    . = 0x3C0;
    KEEP(*(.DoubleExceptionHandler.text));
    . = 0x400;
    *(.userVector)
    *(.*_vector.literal)

    . = ALIGN (16);
    *(.entry.text)
    *(.init.literal)
    *(.init)
    _init_end = ABSOLUTE(.);
  } > iram_0

  .iram0.text :
  {
    /* Code marked as running out of IRAM */

    _iram_text_start = ABSOLUTE(.);
    *(.iram1 .iram1.*)
    *(.iram*)
    *librtc.a:(.literal .text .literal.* .text.*)
    *libarch.a:esp32_spiflash.*(.literal .text .literal.* .text.*)
    *(.wifirxiram .wifirxiram.*)
    *(.wifi0iram  .wifi0iram.*)
    *(.wifislpiram .wifislpiram.*)
    *(.wifislprxiram .wifislprxiram.*)
    *(.phyiram .phyiram.*)

    _iram_text_end = ABSOLUTE(.);

    /* IRAM heap starts at the end of iram */

    . = ALIGN (4);
    _siramheap = ABSOLUTE(.);
  } > iram_0

  /* Shared RAM */

  .dram0.bss (NOLOAD) :
  {
    /* .bss initialized on power-up */

    . = ALIGN (8);
    _sbss = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    KEEP (*(.bss))
    *(.bss.*)
    *(.share.mem)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *libarch.a:esp32_spiflash.*(.bss  .bss.*  COMMON)
    . = ALIGN(8);
    __init_stack_s = ABSOLUTE(.);
    . += 0x1000;
    __init_stack_e = ABSOLUTE(.);
    _ebss = .;
    . = ALIGN(8);
    _ebss = ABSOLUTE(.);
  } >dram

  .dram0.data :
  {
    /* .data initialized on power-up in ROMed configurations. */

    _sdata = ABSOLUTE(.);
    KEEP (*(.data))
    KEEP (*(.data.*))
    KEEP (*(.gnu.linkonce.d.*))
    KEEP (*(.data1))
    KEEP (*(.sdata))
    KEEP (*(.sdata.*))
    KEEP (*(.gnu.linkonce.s.*))
    KEEP (*(.sdata2))
    KEEP (*(.sdata2.*))
    KEEP (*(.gnu.linkonce.s2.*))
    KEEP (*(.jcr))
    *(.dram1 .dram1.*)
    *libphy.a:(.rodata  .rodata.*)
    *libarch.a:esp32_spiflash.*(.rodata  .rodata.*)
    . = ALIGN(4);
    _edata = ABSOLUTE(.);

    /* Heap starts at the end of .data */

    __heap_start = ABSOLUTE(.);
    . += 0x24000;
    . = ALIGN(4);
    __heap_end = ABSOLUTE(.);
  } >dram

  /* External memory bss, from any global variable with EXT_RAM_ATTR attribute */

  .extmem.bss (NOLOAD) :
  {
    _sbss_extmem = ABSOLUTE(.);
    *(.extmem.bss .extmem.bss.*)
    . = ALIGN(4);
    _ebss_extmem = ABSOLUTE(.);
  } > extmem

  .flash.rodata :
  {
    _srodata = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
    *(.xt_except_table)
    *(.gcc_except_table)
    *(.gcc_except_table.*)
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    *(.eh_frame)

    . = (. + 3) & ~ 3;

    /* C++ constructor and destructor tables, properly ordered: */

    _sinit = ABSOLUTE(.);
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    _einit = ABSOLUTE(.);
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))

    /* C++ exception handlers table: */

    __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    _erodata = ABSOLUTE(.);
    /* Literals are also RO data. */
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
    . = ALIGN(4);
  } >drom

  .flash.text :
  {
    _stext = .;
    __text_start = ABSOLUTE(.);
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.fini.literal)
    *(.fini)
    *(.gnu.version)
    . = ALIGN(4);
    __zinitcall_bsp_start = .;
    KEEP (*(.zinitcall.bsp0.init))
    KEEP (*(.zinitcall.bsp1.init))
    KEEP (*(.zinitcall.bsp2.init))
    KEEP (*(.zinitcall.bsp3.init))
    KEEP (*(.zinitcall.bsp4.init))
    __zinitcall_bsp_end = .;
    __zinitcall_device_start = .;
    KEEP (*(.zinitcall.device0.init))
    KEEP (*(.zinitcall.device1.init))
    KEEP (*(.zinitcall.device2.init))
    KEEP (*(.zinitcall.device3.init))
    KEEP (*(.zinitcall.device4.init))
    __zinitcall_device_end = .;
    __zinitcall_core_start = .;
    KEEP (*(.zinitcall.core0.init))
    KEEP (*(.zinitcall.core1.init))
    KEEP (*(.zinitcall.core2.init))
    KEEP (*(.zinitcall.core3.init))
    KEEP (*(.zinitcall.core4.init))
    __zinitcall_core_end = .;
    __zinitcall_sys_service_start = .;
    KEEP (*(.zinitcall.sys.service0.init))
    KEEP (*(.zinitcall.sys.service1.init))
    KEEP (*(.zinitcall.sys.service2.init))
    KEEP (*(.zinitcall.sys.service3.init))
    KEEP (*(.zinitcall.sys.service4.init))
    __zinitcall_sys_service_end = .;
    __zinitcall_sys_feature_start = .;
    KEEP (*(.zinitcall.sys.feature0.init))
    KEEP (*(.zinitcall.sys.feature1.init))
    KEEP (*(.zinitcall.sys.feature2.init))
    KEEP (*(.zinitcall.sys.feature3.init))
    KEEP (*(.zinitcall.sys.feature4.init))
    __zinitcall_sys_feature_end = .;
    __zinitcall_run_start = .;
    KEEP (*(.zinitcall.run0.init))
    KEEP (*(.zinitcall.run1.init))
    KEEP (*(.zinitcall.run2.init))
    KEEP (*(.zinitcall.run3.init))
    KEEP (*(.zinitcall.run4.init))
    __zinitcall_run_end = .;
    __zinitcall_app_service_start = .;
    KEEP (*(.zinitcall.app.service0.init))
    KEEP (*(.zinitcall.app.service1.init))
    KEEP (*(.zinitcall.app.service2.init))
    KEEP (*(.zinitcall.app.service3.init))
    KEEP (*(.zinitcall.app.service4.init))
    __zinitcall_app_service_end = .;
    __zinitcall_app_feature_start = .;
    KEEP (*(.zinitcall.app.feature0.init))
    KEEP (*(.zinitcall.app.feature1.init))
    KEEP (*(.zinitcall.app.feature2.init))
    KEEP (*(.zinitcall.app.feature3.init))
    KEEP (*(.zinitcall.app.feature4.init))
    __zinitcall_app_feature_end = .;
    __zinitcall_test_start = .;
    KEEP (*(.zinitcall.test0.init))
    KEEP (*(.zinitcall.test1.init))
    KEEP (*(.zinitcall.test2.init))
    KEEP (*(.zinitcall.test3.init))
    KEEP (*(.zinitcall.test4.init))
    __zinitcall_test_end = .;
    __zinitcall_exit_start = .;
    KEEP (*(.zinitcall.exit0.init))
    KEEP (*(.zinitcall.exit1.init))
    KEEP (*(.zinitcall.exit2.init))
    KEEP (*(.zinitcall.exit3.init))
    KEEP (*(.zinitcall.exit4.init))
    __zinitcall_exit_end = .;

    __text_end = ABSOLUTE(.);
    _etext = .;
  } >iram_1

}

PROVIDE ( uartAttach = 0x40008fd0 );
PROVIDE ( uart_baudrate_detect = 0x40009034 );
PROVIDE ( uart_buff_switch = 0x400093c0 );
PROVIDE ( UartConnCheck = 0x40008738 );
PROVIDE ( UartConnectProc = 0x40008a04 );
PROVIDE ( UartDev = 0x3ffe019c );
PROVIDE ( uart_div_modify = 0x400090cc );
PROVIDE ( UartDwnLdProc = 0x40008ce8 );
PROVIDE ( UartGetCmdLn = 0x40009564 );
PROVIDE ( Uart_Init = 0x40009120 );
PROVIDE ( UartRegReadProc = 0x40008a58 );
PROVIDE ( UartRegWriteProc = 0x40008a14 );
PROVIDE ( uart_rx_intr_handler = 0x40008f4c );
PROVIDE ( uart_rx_one_char = 0x400092d0 );
PROVIDE ( uart_rx_one_char_block = 0x400092a4 );
PROVIDE ( uart_rx_readbuff = 0x40009394 );
PROVIDE ( UartRxString = 0x400092fc );
PROVIDE ( UartSetBaudProc = 0x40008aac );
PROVIDE ( UartSpiAttachProc = 0x40008a6c );
PROVIDE ( UartSpiReadProc = 0x40008a80 );
PROVIDE ( uart_tx_flush = 0x40009258 );
PROVIDE ( uart_tx_one_char = 0x40009200 );
PROVIDE ( uart_tx_one_char2 = 0x4000922c );
PROVIDE ( uart_tx_switch = 0x40009028 );
PROVIDE ( uart_tx_wait_idle = 0x40009278 );
