.global ts7800_start
ts7800_start:
  str lr, [sp, #-4]!
  bl main
  ldr lr, [sp], #4
  bx lr
