ENTRY(omap3_start)

SECTIONS
{
  .vectors 0x4020FFC8 : /* Exception vectors. */
  {
    vectors_start = .;
    *(.vectors)
    vectors_end = .;
  }

  . = 0x80000000;

  .text : /* The actual instructions. */
  {
    text_start = .;
    *(.text.init)
    *(.text)
    *(.text.*)
    *(.ksymtab) /* must be last */
    text_end = .;
  }

  .rodata : /* Read-only data */
  {
    *(.rodata)
    *(.rodata.*)
  }

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  }

  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  }

  .data : /* Initialized data. */
  {
    data_start = .;
    *(.data)
    data_end = .;
  }

  .bss : /* Uninitialized data. */
  {
    bss_start = .;
    *(.bss)
    *(COMMON)
    bss_end = .;
  }

  runtime_data_start = ALIGN(0x8000);
}
