#include <stdint.h>
#include <stdio.h>
#include "rpcemu.h"
#include "arm.h"
#include "mem.h"
#include "cp15.h"
extern int arm_exec(void);
static uint32_t run(int dyn, uint32_t n){
    dcache = dyn ? 1 : 0;          /* isblockvalid() keys off dcache */
    resetcodeblocks();
    arm.reg[0]=n; arm.reg[15]=0x10000008; arm.event=0;
    for(int i=0;i<60 && arm.reg[0]!=0;i++) arm_exec();
    /* a few more to settle on the halt */
    for(int i=0;i<4;i++) arm_exec();
    return arm.reg[0];
}
int main(void){
    arm_init(); mem_init();
    machine.model = Model_RPCARM710; machine.iomd_type = IOMDType_IOMD;
    mem_reset(16, 2);
    arm_reset(CPUModel_SA110); initcodeblocks();
    updatemode(0x10 | SUPERVISOR); /* 32-bit mode: code lives at 0x10000000 (RAM) */
    ram00[0]=0xe2500001u; /* SUBS r0,r0,#1 */
    ram00[1]=0x1afffffdu; /* BNE  .-8 (loop) */
    ram00[2]=0xeafffffeu; /* B .   (halt)   */
    uint32_t interp = run(0, 100);
    uint32_t jit    = run(1, 100);
    printf("countdown 100: interp r0=%u  jit r0=%u\n", interp, jit);
    int ok = (interp==0) && (jit==0) && (interp==jit);
    /* second value to be sure it actually ran the loop under the JIT */
    uint32_t interp2 = run(0, 12345);
    uint32_t jit2    = run(1, 12345);
    printf("countdown 12345: interp r0=%u  jit r0=%u\n", interp2, jit2);
    ok = ok && (interp2==0) && (jit2==0);
    printf("%s\n", ok ? "PASS (chaining + integration)" : "FAIL");
    return ok ? 0 : 1;
}
