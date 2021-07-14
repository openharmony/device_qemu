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
  _stext = .;
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
