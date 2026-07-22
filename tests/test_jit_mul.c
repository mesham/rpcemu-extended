#include <stdint.h>
#include <stdio.h>
#include "rpcemu.h"
#include "arm.h"
extern void *codegen_test_compile(uint32_t opcode);
extern void codegen_test_interp(uint32_t opcode);
static long checks=0; static int failures=0;
/* r0=rd/rdhi, r4=rdlo, r1=rm, r2=rs, r3=rn */
static uint32_t mul_op(void){ return (0xeu<<28)|(0u<<16)|(2u<<8)|(0x9u<<4)|1u; }               /* MUL r0,r1,r2 */
static uint32_t mla_op(void){ return (0xeu<<28)|(1u<<21)|(0u<<16)|(3u<<12)|(2u<<8)|(0x9u<<4)|1u; } /* MLA r0,r1,r2,r3 */
static uint32_t umull_op(void){ return (0xeu<<28)|(1u<<23)|(0u<<16)|(4u<<12)|(2u<<8)|(0x9u<<4)|1u; } /* UMULL r4,r0,r1,r2 */
static const uint32_t vals[]={0,1,2,0x7fffffff,0x80000000,0xffffffff,0x10000,0x12345,0xdeadbeef,0xfffe};
#define NV ((int)(sizeof(vals)/sizeof(*vals)))
static void chk(const char*name,uint32_t op,int is_long,uint32_t rm,uint32_t rs,uint32_t rn){
    uint32_t i0,i4,j0,j4; void(*fn)(void);
    arm.reg[1]=rm; arm.reg[2]=rs; arm.reg[3]=rn; arm.reg[0]=0x11111111; arm.reg[4]=0x22222222; arm.event=0;
    codegen_test_interp(op); i0=arm.reg[0]; i4=arm.reg[4];
    arm.reg[1]=rm; arm.reg[2]=rs; arm.reg[3]=rn; arm.reg[0]=0x11111111; arm.reg[4]=0x22222222; arm.event=0; linecyc=0;
    fn=(void(*)(void))codegen_test_compile(op); fn(); j0=arm.reg[0]; j4=arm.reg[4];
    checks++;
    int bad = (i0!=j0) || (is_long && i4!=j4);
    if(bad){ if(failures<20) fprintf(stderr,"MISMATCH %-6s rm=%08x rs=%08x rn=%08x | r0 i=%08x j=%08x r4 i=%08x j=%08x\n",name,rm,rs,rn,i0,j0,i4,j4); failures++; }
}
static void sweep(const char*m){
    printf("--- %s ---\n",m);
    for(int i=0;i<NV;i++)for(int j=0;j<NV;j++){
        chk("MUL",mul_op(),0,vals[i],vals[j],0);
        chk("MLA",mla_op(),0,vals[i],vals[j],vals[(i+j)%NV]);
        chk("UMULL",umull_op(),1,vals[i],vals[j],0);
    }
}
int main(void){
    arm_init(); arm_reset(CPUModel_SA110); initcodeblocks();
    sweep("26-bit mode"); updatemode(0x10|SUPERVISOR); sweep("32-bit mode");
    printf("\n%ld checks, %d failures\n",checks,failures);
    return failures?1:0;
}
