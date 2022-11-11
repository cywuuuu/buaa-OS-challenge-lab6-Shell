#include <trap.h>
#include <env.h>
#include <printf.h>

extern void handle_int();
extern void handle_reserved();
extern void handle_tlb();
extern void handle_sys();
extern void handle_mod();
unsigned long exception_handlers[32];

void trap_init()
{
    int i;

    for (i = 0; i < 32; i++) {
        set_except_vector(i, handle_reserved);
    }

    set_except_vector(0, handle_int);
    set_except_vector(1, handle_mod);
    set_except_vector(2, handle_tlb);
    set_except_vector(3, handle_tlb);
    set_except_vector(8, handle_sys);
}
void *set_except_vector(int n, void *addr)
{
    unsigned long handler = (unsigned long)addr;
    unsigned long old_handler = exception_handlers[n];
    exception_handlers[n] = handler;
    return (void *)old_handler;
}


/*** exercise 4.11 ***/
void
page_fault_handler(struct Trapframe *tf)
{
    struct Trapframe PgTrapFrame;
    extern struct Env *curenv;
	//放在pg中
    bcopy(tf, &PgTrapFrame, sizeof(struct Trapframe));
/*
负责将当前现场保存在异常处理栈中，并设置epc寄存器的值
*/
// high(empty) -> low(full) 
// 如果本身中断时候sp已经在异常处理栈里面了， 那么我们把sp下压接着在下面一个trapframe
    if (tf->regs[29] >= (curenv->env_xstacktop - BY2PG) &&
        tf->regs[29] <= (curenv->env_xstacktop - 1)) { 
            tf->regs[29] = tf->regs[29] - sizeof(struct  Trapframe);
            bcopy(&PgTrapFrame, (void *)tf->regs[29], sizeof(struct Trapframe));
        } else {
		// 中断时， sp不在xstacktop
            tf->regs[29] = curenv->env_xstacktop - sizeof(struct  Trapframe);
            bcopy(&PgTrapFrame,(void *)curenv->env_xstacktop - sizeof(struct  Trapframe),sizeof(struct Trapframe));
        }
    // TODO: Set EPC to a proper value in the trapframe
	// 使得从中断恢复后能够跳转到env_pgfault_handler域存储的异常处理函数的地址
	tf->cp0_epc = curenv->env_pgfault_handler; 
    return;
}
