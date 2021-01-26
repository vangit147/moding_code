/* ULP Example: using ADC in deep sleep

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This file contains assembly code which runs on the ULP.

   ULP wakes up to run this code at a certain period, determined by the values
   in SENS_ULP_CP_SLEEP_CYCx_REG registers. On each wake up, the program
   measures input voltage on the given ADC channel 'adc_oversampling_factor'
   times. Measurements are accumulated and average value is calculated.
   Average value is compared to the two thresholds: 'low_thr' and 'high_thr'.
   If the value is less than 'low_thr' or more than 'high_thr', ULP wakes up
   the chip from deep sleep.
*/

/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files 
 */
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.S"

//Define variables, which go into .bss section (zero-initialized data)
	.bss
	.global moving_times
	moving_times:
		.long 0
	.global sysrun_times
	sysrun_times:
		.long 0
	.global move_flags
	move_flags:
		.long 0
	.global compare_times
	compare_times:
		.long 0
	.global stack
	stack:
		.skip 100
	.global stackEnd
	stackEnd:
		.long 0

// Code goes into .text section
	.text
	.global entry
entry:
	move r3,stackEnd
// sysrun_times++ and storage in memory
	move r1,sysrun_times
   	ld r0,r1,0
   	add r0,r0,1
   	st r0,r1,0
//compare_times-sysrun_times   if last ALU set overflow flag,jump
//同号相加时可能产生溢出  异号相减可能产生负溢出 (超过数的表示范围)
//guess compare_times>=0 sysrun_times>=0 0-1=-1溢出
   	move r2,compare_times
   	ld r2,r2,0
   	sub r2,r2,r0
   	jump iswake,ov
    psr 
    jump init_mpu

// ac << 8  and put result into r0
// put acl value into r1
// r0 | r1 and put result into r0

// p_ax - r0  and put result into r2
// if  last ALU set overflow flag
// --Y jump movecount
// --N r0 - m_ax and put result into r2
//     if last ALU set overflow flag
//         --Y jump movecount
//		   --N jump comparez
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

// az << 8  and put  result into r0
// put azl value into r1
// r0 | r1 and put result into r0

// p_az - r0  and put result into r2
// if  last ALU set overflow flag
// --Y jump movecount
// --N r0 - m_az and put result into r2
//     if last ALU set overflow flag
//         --Y jump movecount
//		   --N jump exit
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
// halt ULP program
	.global exit
exit:
	 halt

/* Check if the system can be woken up */
// read  x bits(x=0~16) from  RTC_CNTL_LOW_POWER_ST_REG field of RTC_CNTL_RDY_FOR_WAKEUP
// r0 & 1
// if last ALU == 0
//   --Y  jump exit
//   --N  wake the chip  and  halt ULP program
	.global wake_up
wake_up:
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	and r0, r0, 1
	jump exit, eq
/* Wake up the SoC, end program */
	wake
	halt

// set sysrun_times 0 and storage in memory
// if moving_times >= 3
//	  --Y jump ismoveflag
//    --N set move_flags 0 and storage in memory
//  jump cleartimes
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

// moving_times++  and storage in memory
// jump exit
movecount:
	move r1,moving_times
	ld r0,r1,0
	add r0,r0,1
	st r0,r1,0
	jump exit

// if move_flags < 1
//  --Y jump setflag
//  --N jump clear times
ismoveflag:
	move r1,move_flags
	ld r0,r1,0
	jumpr setflag,1,lt
	jump cleartimes

// set move_times 0 and storage in memory
// jump exit
cleartimes:
	move r1,moving_times
	move r0,0
	st r0,r1,0
	jump exit

// set move_flags 1 and storage in memory
// set moving_times 0 and storage in memory
// jump wake_up
setflag:
	move r1,move_flags
	move r0,1
	st r0,r1,0

	move r1,moving_times
	move r0,0
	st r0,r1,0
	jump wake_up



   
   
