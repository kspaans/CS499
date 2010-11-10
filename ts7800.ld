ENTRY(_start)

SECTIONS
{
  . = 0x8000;

  .text 0x8000 : /* The actual instructions. */
  {
    obj/ts7800.o(.text)
    *(.text)
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.rodata.*)
    *(.glue_7)
    *(.glue_7t)
    *(.note.gnu.build-id)
  }

  .data : /* Initialized data. */
  {
    __data_start__ = .;
    *(.data)
    __data_end__ = .;
  }

  .bss : /* Uninitialized data. */
  {
    __bss_start__ = .;
    *(.bss)
    *(COMMON)
    __bss_end__ = .;
  }
}
