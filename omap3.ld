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
    *(.init)
    *(.text)
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.rodata.*)
    *(.glue_7)
    *(.glue_7t)
    build_note = .;
    build_sha1 = . + 0x10;
    *(.note.gnu.build-id)
    *(.ksymtab) /* must be last */
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
