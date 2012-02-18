#include <cstring>
#include <cstdio>
#include <cassert>

#include "Cpu32.h"
#include "bits.h"
#include "Cpu32Info.h"
#include "Dis.h"

#define TRACE 1
#define TRACEF(x...) do { if (TRACE) printf(x); } while (0)

#define CYCLE_TERM 20

typedef uint32_t word;
typedef uint16_t hword;
typedef uint8_t byte;

using namespace Cpu32Info;

Cpu32::Cpu32()
{
}

Cpu32::~Cpu32()
{
}

void Cpu32::Reset()
{
	memset(r, 0, sizeof(r));
	pc = 0;
}

void Cpu32::Run()
{
	uint64_t cycle = 0;

	assert(mem);

	for (;; cycle++) {
		word ins = mem->Read(pc & ~0x3);
		
		TRACEF("PC 0x%08x ins 0x%08x %s\n", pc, ins, Dis::Dissassemble(ins).c_str());

		pc += 4;

		switch (DecodeForm(ins)) {
			case FORM_IMM_UNSHIFTED: {
				word res;
				word b; 
				
				b = Decodeimm16_signed(ins);
				goto alucommon;
			case FORM_REG_UNSHIFTED:
				b = r[DecodeRb(ins)]; 
alucommon:
				word a = r[DecodeRa(ins)];
				if (DecodeOp(ins) == 0) { // alu op
					switch (DecodeALUOp(ins)) {
						case OP_ADD_NUM: res = a + b; break;
						case OP_SUB_NUM: res = a - b; break;
						case OP_RSB_NUM: res = b - a; break;
						case OP_AND_NUM: res = a & b; break;
						case OP_OR_NUM:  res = a | b; break;
						case OP_XOR_NUM: res = a ^ b; break;
						case OP_LSL_NUM: res = a << b; break;
						case OP_LSR_NUM: res = a >> b; break;
						case OP_ASR_NUM: res = (int)a >> b; break;
						case OP_MOV_NUM: res = b; break;
						case OP_MVB_NUM: res = b & 0xffff; break;
						case OP_MVT_NUM: res = a | (b << 16); break;
						case OP_SLT_NUM: res = a < b; break;
						case OP_SLTE_NUM: res = a <= b; break;
						case OP_SEQ_NUM: res = a == b; break;
						default:
							goto undefined;
					}

					r[DecodeRd(ins)] = res;
				} else if (DecodeOp(ins) == 1) { // load
					res = mem->Read(a + b);
					r[DecodeRd(ins)] = res;
				} else if (DecodeOp(ins) == 2) { // store
					mem->Write(a + b, r[DecodeRd(ins)]);
				} else {
					goto undefined;
				}

				break;
			}
			case FORM_BRANCH_UNSHIFTED:
				word target;

				if (BIT(ins, BRANCH_R)) {
					target = r[DecodeRa(ins)];
				} else {
					target = pc + Decodeimm22_signed(ins);
				}

				// if it's a conditional branch, test and drop out if it fails
				if (BIT(ins, BRANCH_C)) {
					word test = r[DecodeRd(ins)];
					if (BIT(ins, BRANCH_N)) {
						if (test) goto branch_done;
					} else {
						if (!test) goto branch_done;
					}
				}

				if (BIT(ins, BRANCH_L)) {
					r[LR] = pc + 1;
				}

				pc = target;
branch_done:

				break;
			default:
				goto undefined;
		}

		TRACEF("\tR0 0x%08x R1 0x%08x R2  0x%08x R3  0x%08x R4  0x%08x R5  0x%08x R6  0x%08x R7  0x%08x\n", 
			r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
		TRACEF("\tR8 0x%08x R9 0x%08x R10 0x%08x R11 0x%08x R12 0x%08x R13 0x%08x R14 0x%08x R15 0x%08x\n", 
			r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);

//		if ((cycle % 1000000) == 0)
//			printf("%lld cycles\n", cycle);

		if (cycle == CYCLE_TERM)
			break;

		continue;

undefined:
		printf("UNDEFINED!\n");
	}
}

