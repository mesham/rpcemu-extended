/*
  RPCEmu - differential test for the amd64 recompiler's native flag generation.

  For every flag-setting data-processing / compare instruction that the JIT now
  recompiles inline, this runs the same instruction on the same inputs through
  both the interpreter opcode handler (the reference) and a freshly-recompiled
  native block, and asserts the resulting NZCV flags (and destination register)
  are identical. Covers carry/overflow edge cases and both 26- and 32-bit modes.

  Built only when RPCEMU_JIT_TEST is defined (see tests/CMakeLists.txt).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rpcemu.h"
#include "arm.h"

/* Test-only hooks from arm_dynarec.c / codegen_amd64.c. */
extern void *codegen_test_compile(uint32_t opcode);
extern void codegen_test_interp(uint32_t opcode);
extern unsigned long codegen_test_flag_emits; /* bumped by gen_native_flags() */

static int not_recompiled = 0;

/* ARM opcode field values (bits 24..21). */
enum { OP_AND, OP_EOR, OP_SUB, OP_RSB, OP_ADD, OP_ADC, OP_SBC, OP_RSC,
       OP_TST, OP_TEQ, OP_CMP, OP_CMN, OP_ORR, OP_MOV, OP_BIC, OP_MVN };

#define COND_AL 0xeu

#define RN_NUM 1
#define RM_NUM 2
#define RD_NUM 0

static const uint32_t RD_SENTINEL = 0x5a5aa5a5u;

/* Interesting operand values: carry/overflow/sign boundaries. */
static const uint32_t vals[] = {
	0x00000000u, 0x00000001u, 0x00000002u, 0x7fffffffu,
	0x80000000u, 0xffffffffu, 0x40000000u, 0xfffffffeu,
};
#define NVALS ((int) (sizeof(vals) / sizeof(vals[0])))

static const uint32_t imms[] = { 0x00, 0x01, 0x02, 0x7f, 0x80, 0xff };
#define NIMMS ((int) (sizeof(imms) / sizeof(imms[0])))

static int failures = 0;
static long checks = 0;

static uint32_t
dp_reg(int op, int rn, int rd, int rm)
{
	return (COND_AL << 28) | ((uint32_t) op << 21) | (1u << 20) /* S */
	     | ((uint32_t) rn << 16) | ((uint32_t) rd << 12) | (uint32_t) rm;
}

static uint32_t
dp_imm(int op, int rn, int rd, uint32_t imm8)
{
	return (COND_AL << 28) | (1u << 25) /* I */ | ((uint32_t) op << 21)
	     | (1u << 20) /* S */ | ((uint32_t) rn << 16) | ((uint32_t) rd << 12)
	     | (imm8 & 0xff);
}

static int
is_compare(int op)
{
	return op == OP_TST || op == OP_TEQ || op == OP_CMP || op == OP_CMN;
}

/*
 * Run one opcode with the given operands through interpreter and JIT and
 * compare. `a` -> Rn, `b` -> Rm, `cin` -> carry in.
 */
static void
check(const char *name, int op, uint32_t opcode, uint32_t a, uint32_t b, int cin)
{
	void (*fn)(void);
	uint32_t ref_flags, ref_rd, jit_flags, jit_rd;
	uint32_t base_cpsr;
	const int rd = (int) ((opcode >> 12) & 0xf);

	/* Establish a clean flag word with the requested carry-in, preserving
	   the non-NZCV bits (PC/mode in 26-bit, control bits in 32-bit). */
	base_cpsr = arm.reg[cpsr] & 0x0fffffffu;
	if (cin) {
		base_cpsr |= CFLAG;
	}

	/* --- interpreter reference --- */
	arm.reg[RN_NUM] = a;
	arm.reg[RM_NUM] = b;
	if (rd != RN_NUM && rd != RM_NUM) {
		arm.reg[rd] = RD_SENTINEL;
	}
	arm.reg[cpsr] = base_cpsr;
	codegen_test_interp(opcode);
	ref_flags = arm.reg[cpsr] & 0xf0000000u;
	ref_rd = arm.reg[rd];

	/* --- recompiled native block --- */
	arm.reg[RN_NUM] = a;
	arm.reg[RM_NUM] = b;
	if (rd != RN_NUM && rd != RM_NUM) {
		arm.reg[rd] = RD_SENTINEL;
	}
	arm.reg[cpsr] = base_cpsr;
	linecyc = 0; /* force the block to return after the single instruction */
	{
		const unsigned long emits_before = codegen_test_flag_emits;
		fn = (void (*)(void)) codegen_test_compile(opcode);
		/* Confirm the inline native flag path was emitted; otherwise the
		   block fell back to the C handler and any match is vacuous. */
		if (codegen_test_flag_emits == emits_before) {
			not_recompiled++;
			if (not_recompiled <= 10) {
				fprintf(stderr, "NOT RECOMPILED %-5s op=%08x\n", name, opcode);
			}
		}
	}
	fn();
	jit_flags = arm.reg[cpsr] & 0xf0000000u;
	jit_rd = arm.reg[rd];

	checks++;

	if (ref_flags != jit_flags || ref_rd != jit_rd) {
		if (failures < 40) {
			fprintf(stderr,
			    "MISMATCH %-5s op=%08x Rn=%08x Rm=%08x Cin=%d : "
			    "flags ref=%08x jit=%08x", name, opcode, a, b, cin,
			    ref_flags, jit_flags);
			if (!is_compare(op)) {
				fprintf(stderr, " Rd ref=%08x jit=%08x", ref_rd, jit_rd);
			}
			fprintf(stderr, "\n");
		}
		failures++;
	}
}

static void
sweep(const char *name, int op)
{
	int i, j, c;

	/* Register form, full operand matrix. */
	for (i = 0; i < NVALS; i++) {
		for (j = 0; j < NVALS; j++) {
			for (c = 0; c < 2; c++) {
				check(name, op, dp_reg(op, RN_NUM, RD_NUM, RM_NUM),
				      vals[i], vals[j], c);
			}
		}
	}

	/* Immediate form (rotate 0), Rn from the full matrix. Test both Rd != Rn
	   and Rd == Rn (the latter exercises the read-modify-write code path). */
	for (i = 0; i < NVALS; i++) {
		for (j = 0; j < NIMMS; j++) {
			for (c = 0; c < 2; c++) {
				check(name, op, dp_imm(op, RN_NUM, RD_NUM, imms[j]),
				      vals[i], 0, c);
				check(name, op, dp_imm(op, RN_NUM, RN_NUM, imms[j]),
				      vals[i], 0, c);
			}
		}
	}
}

static void
run_all(const char *mode_name)
{
	printf("--- %s ---\n", mode_name);

	/* Arithmetic + compares. */
	sweep("ADDS", OP_ADD);
	sweep("SUBS", OP_SUB);
	sweep("RSBS", OP_RSB);
	sweep("CMP",  OP_CMP);
	sweep("CMN",  OP_CMN);

	/* Logical / move / test (no-shift / rotate-0 forms are recompiled). */
	sweep("ANDS", OP_AND);
	sweep("EORS", OP_EOR);
	sweep("ORRS", OP_ORR);
	sweep("BICS", OP_BIC);
	sweep("MOVS", OP_MOV);
	sweep("MVNS", OP_MVN);
	sweep("TST",  OP_TST);
	sweep("TEQ",  OP_TEQ);
}

int
main(void)
{
	arm_init();
	arm_reset(CPUModel_SA110);
	initcodeblocks();

	/* arm_reset leaves us in 26-bit SUPERVISOR mode. */
	run_all("26-bit mode");

	/* Switch to 32-bit SUPERVISOR mode and repeat. */
	updatemode(0x10 | SUPERVISOR);
	run_all("32-bit mode");

	printf("\n%ld checks, %d failures, %d not-recompiled\n",
	       checks, failures, not_recompiled);
	return (failures || not_recompiled) ? 1 : 0;
}
