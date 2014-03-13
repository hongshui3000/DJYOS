 @----------------------------------------------------
 @Copyright (C)  2005-2008   lst.
 @��Ȩ���� (C)  2005-2008    lst.
 @����ģ��:  CPU��ʼ��
 @���ߣ�     lst
 @�汾��      V1.1.0
 @�ļ�����:  CPU��ʼ�������û������ʵ�ֵĲ���
 @����˵��:  ��
 @�޶���ʷ:
 @2. ����: 2009-10-30
 @   ����: lst
 @   �°汾�ţ�V1.1.0
 @   �޸�˵��: ���ڴ��ʱ����C���Գ�ʼ��
 @1. ����: 2009-03-10
 @   ����: lst
 @   �°汾�ţ�V1.0.0
 @   �޸�˵��: s3c2440��ԭʼ�汾
 @------------------------------------------------------
.cpu	arm926ej-s
.fpu	softvfp
.file	"initcpu.S"

#include "arm_mode_s_file.h"

.section .text._start, "ax", %progbits
.align  2
.global _start

.long 0x11
_start:        /* ��λ������ַ */
	ldr		r0, =0x68000010		@cs2(flash)���õ�ַ
	ldr		r1, =0x00a0050d		@cs2����Ϊ13���������ڣ�Ĭ��ֵ��88�����ڣ�����̫��
	str		r1, [r0]
	
    mrs     r0, cpsr                  /* ȡCPSR */
    bic     r0, r0, #SF_MODEMASK   /* ��ģʽλ */
    orr     r1, r0, #SF_SVCMODE|SF_IRQ_BIT|SF_FIQ_BIT    /* ����Ϊ����̬������ֹ�ж� */
    msr     cpsr_cxsf,r1              /* �л�������̬,�ɷ�ֹ���ⷵ��0��ַʱ����. */
	LDR     r0, =0xffff2000
	mov		sp, r0

    mrc     p15,0,r0,c1,c0,0
    orr     r0,r0,#0x00002000       @ʹ�ܸ߶�����
    bic     r0,r0,#0x1000           @��ֹIcache
    bic     r0,r0,#0x1              @��ֹmmu
    bic     r0,r0,#0x4              @��ֹDcache
    mcr     p15,0,r0,c1,c0,0

#ifdef release
	BL	    config_clocks
	BL	    config_ddr
#endif
	
/* ��ʼ��ջָ�� */
    mrs     r0, cpsr
    bic     r0, r0, #SF_MODEMASK

    orr		r0, r0, #SF_IRQ_BIT|SF_FIQ_BIT

    orr     r1, r0, #SF_UNDEFMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =undef_top

    orr     r1, r0, #SF_ABORTMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =abort_top

    orr     r1, r0, #SF_IRQMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =irq_top

    orr     r1, r0, #SF_FIQMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =fiq_top

    orr     r1, r0, #SF_SVCMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =svc_top

    orr     r1, r0, #SF_SYSMODE
    msr     cpsr_cxsf, r1
	
	LDR     r0, =0xffff2000
	mov		sp, r0
    bl		init_mmu
    ldr     sp, =usr_top

    bl      load_preload
    
@----������-----------------------------------------------------------------
@����: ��������ַ������DSP��ARM����λ
@����: ��
@����: ������
@����ԭ��:void __asm_hot_restart(void );
@-----------------------------------------------------------------------------
.section .text.__asm_hot_restart, "ax", %progbits
.global __asm_hot_restart
__asm_hot_restart:
    mrs     r0, cpsr                  /* ȡCPSR */
    bic     r0, r0, #SF_MODEMASK   /* ��ģʽλ */
    orr     r1, r0, #SF_SVCMODE|SF_IRQ_BIT|SF_FIQ_BIT    /* ����Ϊ����̬������ֹ�ж� */
    msr     cpsr_cxsf,r1              /* �л�������̬,�ɷ�ֹ���ⷵ��0��ַʱ����. */
	LDR     r0, =0xffff2000
	mov		sp, r0

/* ��ʼ��ջָ�� */
    mrs     r0, cpsr
    bic     r0, r0, #SF_MODEMASK

    orr		r0, r0, #SF_IRQ_BIT|SF_FIQ_BIT

    orr     r1, r0, #SF_UNDEFMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =undef_top

    orr     r1, r0, #SF_ABORTMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =abort_top

    orr     r1, r0, #SF_IRQMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =irq_top

    orr     r1, r0, #SF_FIQMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =fiq_top

    orr     r1, r0, #SF_SVCMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =svc_top

    orr     r1, r0, #SF_SYSMODE
    msr     cpsr_cxsf, r1
    ldr     sp, =usr_top
	
    bl      load_preload

@ end of __asm_hot_restart

.end