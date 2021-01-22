       
       
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
    .global ac,ay,az,acl,azl,p_ax,p_az,m_ax,m_az
ac: .long 0
ay: .long 0
az: .long 0
acl:.long 0
azl:.long 0
p_ax:.long 0
p_az:.long 0
m_ax:.long 0
m_az:.long 0
.text
.global init_mpu
init_mpu:
 MOVE r1, 0X68
 push r1
 move r1, 0X6B
 push r1
 move r1, 0x80
 push r1
    psr
 JUMP write8
    ADD r3,r3,3
 move r2,500
 psr
 jump waitMs
    MOVE r1, 0X68
 push r1
 move r1, 0X6B
 push r1
 move r1, 0x00
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
    psr
 jump didinit
 ret
sleepmode:
    MOVE r1, 0X68
 push r1
 move r1, 0X6B
 push r1
 move r1, 0x41
 push r1
    psr
 JUMP write8
    ADD r3,r3,3
 ret
waitMs:
   wait 8000
   sub r2,r2,1
   jump doneWaitMs,eq
   jump waitMs
doneWaitMs:
   ret
fail:
 move r2,1
 ret
didinit:
 MOVE r1, 0X68
 push r1
 move r1, 0X38
 push r1
 move r1, 0x00
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
 MOVE r1, 0X68
 push r1
 move r1, 0X19
 push r1
 move r1,50
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
 MOVE r1, 0X68
 push r1
 move r1, 0X1A
 push r1
 move r1, 25
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
 MOVE r1, 0X68
 push r1
 move r1, 0X6B
 push r1
 move r1, 0x01
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
 MOVE r1, 0X68
 push r1
 move r1, 0X6C
 push r1
 move r1, 0x07
 push r1
    psr
 JUMP write8
 ADD r3,r3,3
    psr
    jump read
  read:
    move r2,500
 psr
 jump waitMs
    move r1, 0X68
    push r1
    MOVE r1, 0x3b
    push r1
    push r1
    psr
    jump read_intro
 add r3,r3,3
 move r2,0
 psr
 jump i2c_read_byte
 move r2, ac
    st r0,r2,0
 move r2,0
 psr
 jump i2c_read_byte
 move r2, acl
 st r0,r2,0
 move r2,0
 psr
 jump i2c_read_byte
 move r2,0
 psr
 jump i2c_read_byte
 move r2,0
 psr
 jump i2c_read_byte
 move r2, az
 st r0,r2,0
 move r2,0
 psr
 jump i2c_read_byte
 move r2, azl
 st r0,r2,0
    psr
 jump i2c_stop_cond
 jump sleepmode
    jump comparex
