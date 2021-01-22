       
       
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
.text
write_intro:
 psr
 jump i2c_start_cond
 ld r2,r3,20
 lsh r2,r2,1
 psr
 jump i2c_write_byte
 jumpr popfail,1,ge
 ld r2,r3,16
 psr
 jump i2c_write_byte
 jumpr popfail,1,ge
 ret
.global write8
write8:
 psr
 jump write_intro
write_b:
 ld r2,r3,8
 psr
 jump i2c_write_byte
 jumpr fail,1,ge
 psr
 jump i2c_stop_cond
 move r2,0
 ret
.global read_intro
read_intro:
       psr
       jump i2c_start_cond
       ld r2,r3,16
       lsh r2,r2,1
    psr
    jump i2c_write_byte
       jumpr popfail,1,ge
       ld r2,r3,12
       psr
       jump i2c_write_byte
       jumpr popfail,1,ge
       psr
       jump i2c_start_cond
       ld r2,r3,16
       lsh r2,r2,1
       or r2,r2,1
       psr
       jump i2c_write_byte
       jumpr popfail,1,ge
       ret
popfail:
 pop r1
 move r2,1
 ret
fail:
 move r2,1
 ret
