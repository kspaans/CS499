.global ts7800_start
ts7800_start:
  ldr sp, =init_stack
  bl main
  b reboot

reboot:
  ldr r0, =0xf1020000
  ldr r1, [r0, #0x108]
  orr r1, r1, #4
  str r1, [r0, #0x108]
  ldr r1, [r0, #0x10c]
  orr r1, r1, #1
  str r1, [r0, #0x10c]
  b .
