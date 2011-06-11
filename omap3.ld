ENTRY(omap3_start)

SECTIONS
{
  . = 0x80000000;

  .text : /* The actual instructions. */
  {
    *(.init)
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
    *(.data)
  }

  .bss : /* Uninitialized data. */
  {
    *(.bss)
    *(COMMON)
  }

  /DISCARD/ :
  {
    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidx.*)
    *(.ARM.extab*)
  }

  runtime_data_start = .;

  .vectors 0x4020FFC8 : /* Exception vectors. */
  {
    *(.vectors)
  }
}
