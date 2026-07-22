#include <stdint.h>
#include <stdio.h>
#include "rpcemu.h"
#include "arm.h"
extern void *codegen_test_compile(uint32_t opcode);
extern void codegen_test_interp(uint32_t opcode);
static long checks=0; static int failures=0;
static uint32_t bop(int link,uint32_t off24){return (0xeu<<28)|(0x5u<<25)|((uint32_t)link<<24)|(off24&0xffffff);}
static void chk(int link,uint32_t off24,uint32_t r15){
    uint32_t op=bop(link,off24), ir15,ir14,jr15,jr14; void(*fn)(void);
    uint32_t base=arm.reg[cpsr]&0x0fffffff;
    arm.reg[15]=r15; arm.reg[14]=0xdeadbeef; arm.event=0;
    codegen_test_interp(op);
    arm.reg[15] += 4; /* pipeline advance the real interp loop applies per instruction */
    ir15=arm.reg[15]; ir14=arm.reg[14];
    arm.reg[15]=r15; arm.reg[14]=0xdeadbeef; arm.event=0; linecyc=0;
    fn=(void(*)(void))codegen_test_compile(op); fn(); jr15=arm.reg[15]; jr14=arm.reg[14];
    checks++;
    if((ir15!=jr15)||(link&&ir14!=jr14)){
        if(failures<30) fprintf(stderr,"MISMATCH %s off=%06x r15=%08x | R15 i=%08x j=%08x  R14 i=%08x j=%08x\n",
            link?"BL":"B ",off24,r15,ir15,jr15,ir14,jr14);
        failures++;
    }
    (void)base;
}
static void sweep(const char*m){
    printf("--- %s ---\n",m);
    uint32_t offs[]={0,1,2,0x100,0x7fffff,0x800000,0xffffff,0x400000,0x000010};
    uint32_t r15s[]={0x00008000,0x00010000,0x00040000,0x01000000,0x0000000c};
    for(int link=0;link<2;link++)
      for(unsigned o=0;o<sizeof(offs)/sizeof(*offs);o++)
        for(unsigned r=0;r<sizeof(r15s)/sizeof(*r15s);r++)
          chk(link,offs[o],r15s[r]);
}
int main(void){
    arm_init(); arm_reset(CPUModel_SA110); initcodeblocks();
    sweep("26-bit mode");
    updatemode(0x10|SUPERVISOR);
    sweep("32-bit mode");
    printf("\n%ld checks, %d failures\n",checks,failures);
    return failures?1:0;
}
