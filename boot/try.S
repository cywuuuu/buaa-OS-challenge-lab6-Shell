#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

main:
	li s0, 10
	li t0, 0
	for_t0_begin:
	slt t1, t0, s0
	beq t1, zero, for_t0_end
	
	addi t0, t0, 1
	j for_t0_end

