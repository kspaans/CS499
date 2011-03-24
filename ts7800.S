.global ts7800_start
ts7800_start:
  str lr, [sp, #-4]!
  bl main
  ldr lr, [sp], #4

  @ To return to U-Boot you should first return things to the state
  @ you found them: svc mode, interrupts disabled, etc. If you don't
  @ want to bother, reboot instead. U-Boot lives at 0x07FCA000 so
  @ don't overwrite anything above the stack.
  bx lr

ts7800_reboot:
  ldr r0, =0xf1020000
  ldr r1, [r0, #0x108]
  orr r1, r1, #4
  str r1, [r0, #0x108]
  ldr r1, [r0, #0x10c]
  orr r1, r1, #1
  str r1, [r0, #0x10c]
  b .
