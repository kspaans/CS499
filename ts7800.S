.global ts7800_start
ts7800_start:
  ldr sp, =init_stack
  bl zero_bss
  bl main
  b reboot

zero_bss:
  mov r3, #0
  ldr r1, =__bss_start__
  ldr r2, =__bss_end__

zero_bss_loop:
  cmp r1, r2
  bxhs lr
  strb r3, [r1], #1
  b zero_bss_loop

reboot:
  ldr r0, =0xf1020000
  ldr r1, [r0, #0x108]
  orr r1, r1, #4
  str r1, [r0, #0x108]
  ldr r1, [r0, #0x10c]
  orr r1, r1, #1
  str r1, [r0, #0x10c]
  b .
