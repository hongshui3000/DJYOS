/*
@----------------------------------------------------
@Copyright (C), 2004-2009,  lst.
@版权所有 (C), 2004-2009,   lst.
@所属模块:调度器
@作者：lst
@版本：V1.0.0
@文件描述:调度器中与CPU直接相关的汇编代码，主要是线程切换方面的代码。
@其他说明:
@修订历史:
@2. ...
@1. 日期: 2009-01-04
@   作者: lst
@   新版本号: V1.0.0
@   修改说明: 原始版本
@------------------------------------------------------
@ 注: psp并不保存在栈中，而是在vm->stack中!!
*/

.syntax		unified
.cpu		cortex-m3
.fpu		softvfp
.thumb
.file	"cpus.S"

/*
@----重置线程-----------------------------------------------------------------
@功能:  初始化线程的栈，并挂上线程执行函数，新创建线程时将会调用本函数
@参数:  thread_routine，被重置的线程函数
@       vm，线程指针
@返回:  初始化结束后的当前栈指针
@函数原型:void * __asm_reset_thread(void (*thread_routine)(struct event_script *),
@                                        struct  thread_vm  *vm)@
@-----------------------------------------------------------------------------
*/
.text
.align 2
.global __asm_reset_thread
.thumb
.thumb_func
__asm_reset_thread:
    ldr     r2,[r1,#4]          /* 取线程栈顶指针 */
    mov     r12,#0x01000000     /* xpsr的初始值 */
    ldr     r3,=__djy_vm_engine /* 取线程引擎指针 */
    stmfd   r2!,{r3,r12}        /* 存pc，xpsr */
    sub     r2,r2,#14*4         /* 后退14个寄存器,初始状态r0-r12中，除r0外均无意义， */
                                /* __vm_engine函数不返回，lr也无意义 */
    /* 存在r0中的thread_routine是__vm_engine的参数，切换上下文时，thread_routine将
        恢复到r0中，根据调用约定，r0的值就是__vm_engine函数的参数。 */
    str     r0,[r2,#8*4]        /* 保存 thread_routine指针到r0的位置. */
    str     r2,[r1]             /* 保存vm的当前栈指针到vm->stack中 */
    bx      lr

/*
@----复位老线程，切换到新线程-------------------------------------------------
@功能:  把old_vm的上下文重新初始化到新创建的状态，然后切换到新线程的
@       上下文中继续执行。
@       当一个在常驻内存的线程中处理的事件完成后，不能删除该线程，必须复位
@       该线程，然后切换到其他就绪线程，这时候就需要调用本函数。因此本函数在
@       被复位的线程上下文里面执行。
@参数:  thread_routine，被重置的线程函数
@       new_vm，目标线程的线程
@       old_vm，被复位的线程
@返回:  无
@函数原型:void __asm_reset_switch(void (*thread_routine)(void),
@                           struct  thread_vm *new_vm,struct  thread_vm *old_vm)@
@-----------------------------------------------------------------------------
*/
.text
.align 2
.global __asm_reset_switch
.thumb
.thumb_func
__asm_reset_switch:
    ldr     sp,[r2,#4]          /* 取老线程栈顶指针 */
    mov     r12,#0x01000000     /* xpsr的初始值 */
    ldr     r11,=__djy_vm_engine    /* 取线程引擎指针 */
    push    {r11,r12}           /* 存pc,xpsr */
    sub     sp,sp,#14*4         /* 后退14个寄存器,初始状态r0-r12中，除r0外均无意义，
                                   __vm_engine函数不返回，lr也无意义，至此，完成老线程复位*/
    str     r0,[sp,#8*4]        /* 保存 thread_routine指针.至此，完成老线程重置 */
    str     sp,[r2]             /* 保存当前栈指针到old_vm->stack中 */
			  
    ldr     sp,[r1]             /* 取得新上下文指针 */
    bl      int_half_enable_asyn_signal /* 对应done函数开头的 __int_reset_asyn_signal 调用 */
    svc     0                   /* 切入新上下文 */

/*
@----启动多事件调度-----------------------------------------------------------
@功能: 系统初始化完成后，启动多事件调度并切入第一个事件.
@参数: new_vm，首次启动的线程控制块指针
@返回: 无
@函数原型: void __asm_start_thread(struct  thread_vm  *new_vm)@
@-----------------------------------------------------------------------------
*/
.text
.align 2
.global __asm_start_thread
.thumb
.thumb_func
__asm_start_thread:
    ldr     r1,=msp_top
    msr     msp,r1			    /* 重置主栈指针，主栈将用于异常和中断 */
    mov     r1,#2
    msr     control,r1          /* 切换到特权线程模式，栈指针用psp */
    ldr     sp,[r0]             /* 取得新上下文指针 */
    bl      int_half_enable_asyn_signal /* 线程启动后，调度应该是允许的 */
    svc     0                   /* 切入新上下文 */

/*
@----切入上下文---------------------------------------------------------------
@功能:  不保存原上下文，直接切入新的上下文执行
@参数:  new_vm，待切入的线程控制块指针
@返回:  无
@函数原型: void __asm_turnto_context(struct  thread_vm  *new_vm)@
@说明:  当事件完成,就没有必要保存旧事件的上下文,直接切换到新事件即可.
@-----------------------------------------------------------------------------
*/
.text
.align 2
.global __asm_turnto_context
.thumb
.thumb_func
__asm_turnto_context:
    ldr     sp,[r0]             /* 取得新上下文指针 */
    bl      int_half_enable_asyn_signal /* 对应done函数开头的 int_save_asyn_signal 调用 */
    svc     0                   /* 切入新上下文 */

/*
@----上下文切换---------------------------------------------------------------
@功能:  保存当前线程的上下文，切换到新线程的上下文中继续执行。
@参数:  new_sp，新上下文的栈指针
@参数:  old_sp，旧上下文的栈指针的指针，即&vm->stack。无需提供旧上下文栈指针，
@               sp寄存器的当前值就是
@返回:  无
@函数原型: void __asm_switch_context(struct  thread_vm *new_vm,struct  thread_vm *old_vm)@
@-----------------------------------------------------------------------------
*/
.text
.align 2
.global __asm_switch_context
.thumb
.thumb_func
__asm_switch_context:
    mrs     r3,xpsr
	orr		r3,#0x01000000      /* xpsr的T标志读不出来，得手工置位。 */
    push    {r3}                /* 保存xpsr */
    push    {lr}                /* 保存PC,从其他线程切换回来时，相当于原线程调用 */
                                /* 本函数返回，故用lr替代pc */
    push    {r0-r3,r12,lr}      /* 保存r0-r3,r12,lr */
    push    {r4-r11}
    str     sp,[r1]             /* 保存旧上下文栈指针到old_vm->stack */

    ldr     sp,[r0]             /* 取得新上下文指针 */
    bl      int_half_enable_asyn_signal
    svc     0                   /* 切入新上下文 */

/*
@----中断中的上下文切换-------------------------------------------------------
@功能:  保存被中断线程的上下文，切换到新线程的上下文中继续执行。本函数虽然在中
@       断服务函数(非用户用int_isr_connect函数连接的中断服务函数)中，但在ARM
@       中，却运行在svc态
@参数:  new_sp，切换目标线程
@参数:  old_sp，被中断线程
@返回:  无
@函数原型: void __asm_switch_context_int(struct thread_vm *new_vm,struct thread_vm *old_vm)@
@-----------------------------------------------------------------------------
*/

.text
.align 2
.global __asm_switch_context_int
.thumb
.thumb_func
__asm_switch_context_int:
    mrs     r2,psp      /* 取被中断线程的psp */
    str     r2,[r1]     /* 把psp写入线程数据结构中 */
    ldr     r2,[r0]     /* 取待切入线程的psp */
    msr     psp,r2      /* 待切入线程的当前栈指针写入psp，中断返回恢复上下文将以此为准 */
    bx      lr

.end
