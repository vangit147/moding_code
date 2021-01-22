       
       
.macro push rx
 st \rx,r3,0
 sub r3,r3,1
.endm
.macro pop rx
 add r3,r3,1
 ld \rx,r3,0
.endm
.macro psr
 .set addr,(.+16)
 move r1,addr
 push r1
.endm
.macro ret
 pop r1
 jump r1
.endm
  .bss
 .global moving_times
     moving_times: .long 0
     .global sysrun_times
     sysrun_times: .long 0
 .global move_flags
     move_flags: .long 0
  .global compare_times
     compare_times: .long 0
 .global stack
stack:
 .skip 100
 .global stackEnd
stackEnd:
 .long 0
 .text
 .global entry
entry:
   move r3,stackEnd
   move r1,sysrun_times
   ld r0,r1,0
   add r0,r0,1
   st r0,r1,0
   move r2,compare_times
   ld r2,r2,0
   sub r2,r2,r0
   jump iswake,ov
    psr
    jump init_mpu
 .global comparex
comparex:
    move r1,ac
 ld r0,r1,0
    lsh r0,r0,8
 move r1,acl
 ld r1,r1,0
 or r0,r1,r0
 move r2,p_ax
 ld r2,r2,0
 sub r2,r2,r0
    jump movecount,ov
 move r2,m_ax
 ld r2,r2,0
 sub r2,r0,r2
    jump movecount,ov
    jump comparez
comparez:
    move r1,az
 ld r0,r1,0
    lsh r0,r0,8
 move r1,azl
 ld r1,r1,0
 or r0,r1,r0
 move r2,p_az
 ld r2,r2,0
 sub r2,r2,r0
    jump movecount,ov
 move r2,m_az
 ld r2,r2,0
 sub r2,r0,r2
    jump movecount,ov
    jump exit
noNegate:
 ret
  .global exit
exit:
  halt
 .global wake_up
wake_up:
 REG_RD ((((0x3ff48000 + 0xc0)) - 0x3ff48000) / 4), ((19) + ((((((((0x1) >> (0)) & 1) + (((0x1) >> (0 + 1)) & 1)) + ((((0x1) >> (0 + 2)) & 1) + (((0x1) >> (0 + 2 + 1)) & 1))) + (((((0x1) >> (0 + 4)) & 1) + (((0x1) >> (0 + 4 + 1)) & 1)) + ((((0x1) >> (0 + 4 + 2)) & 1) + (((0x1) >> (0 + 4 + 2 + 1)) & 1)))) + ((((((0x1) >> (0 + 8)) & 1) + (((0x1) >> (0 + 8 + 1)) & 1)) + ((((0x1) >> (0 + 8 + 2)) & 1) + (((0x1) >> (0 + 8 + 2 + 1)) & 1))) + (((((0x1) >> (0 + 8 + 4)) & 1) + (((0x1) >> (0 + 8 + 4 + 1)) & 1)) + ((((0x1) >> (0 + 8 + 4 + 2)) & 1) + (((0x1) >> (0 + 8 + 4 + 2 + 1)) & 1)))))) - 1), (19)
 and r0, r0, 1
 jump exit, eq
 wake
 halt
iswake:
 move r1,sysrun_times
 move r0,0
 st r0,r1,0
 move r1,moving_times
 ld r0,r1,0
 jumpr ismoveflag,3,ge
 move r1,move_flags
 move r0,0
 st r0,r1,0
    jump cleartimes
movecount:
 move r1,moving_times
 ld r0,r1,0
 add r0,r0,1
 st r0,r1,0
 jump exit
ismoveflag:
 move r1,move_flags
 ld r0,r1,0
 jumpr setflag,1,lt
 jump cleartimes
cleartimes:
 move r1,moving_times
 move r0,0
 st r0,r1,0
 jump exit
setflag:
 move r1,move_flags
 move r0,1
 st r0,r1,0
 move r1,moving_times
 move r0,0
 st r0,r1,0
 jump wake_up
