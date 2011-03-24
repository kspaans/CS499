.global ts7800_start
ts7800_start:
  str lr, [sp, #-4]!
  bl main
  ldr lr, [sp], #4
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
