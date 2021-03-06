#ifndef __TRAP_H__
#define __TRAP_H__

#include "mmu.h"

// Gate descriptors for interrupts and traps
struct gate_desc {
  uint off_15_0 : 16;   // low 16 bits of offset in segment
  uint cs : 16;         // code segment selector
  uint args : 5;        // # args, 0 for interrupt/trap gates
  uint rsv1 : 3;        // reserved(should be zero I guess)
  uint type : 4;        // type(STS_{TG,IG32,TG32})
  uint s : 1;           // must be 0 (system)
  uint dpl : 2;         // descriptor(meaning new) privilege level
  uint p : 1;           // Present
  uint off_31_16 : 16;  // high bits of offset in segment
};

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SET_GATE(gate, istrap, sel, off, d)                \
{                                                         \
  (gate).off_15_0 = (uint)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint)(off) >> 16;                  \
}

struct trap_frame {
	// push stack by alltraps from trapasm.S
	uint edi;
	uint esi;
	uint ebp;
	uint oesp;
	uint edx;
	uint ecx;
	uint ebx;
	uint eax;
	uint gs;
	uint fs;
	uint es;
	uint ds;
	
	// push stack by vector.S
	uint trapno;

	// push stack by x86
	uint errno;
	uint eip;
	uint cs;
	uint eflags;
	uint esp;
	uint ss;
};

void init_idt();
void trap(struct trap_frame *tf);

#endif

