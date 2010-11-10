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
  b .
