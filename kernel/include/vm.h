#ifndef __VM_H__
#define __VM_H__

#include "mmu.h"

#define USER0 (char *)0x1000
#define USER1 (char *)0x2000

// Segment Descriptor
struct gdt_desc {
  uint lim_15_0 : 16;  // Low bits of segment limit
  uint base_15_0 : 16; // Low bits of segment base address
  uint base_23_16 : 8; // Middle bits of segment base address
  uint type : 4;       // Segment type (see STS_ constants)
  uint s : 1;          // 0 = system, 1 = application
  uint dpl : 2;        // Descriptor Privilege Level
  uint p : 1;          // Present
  uint lim_19_16 : 4;  // High bits of segment limit
  uint avl : 1;        // Unused (available for software use)
  uint rsv1 : 1;       // Reserved
  uint db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
  uint g : 1;          // Granularity: limit scaled by 4K when set
  uint base_31_24 : 8; // High bits of segment base address
};

struct kernel_map {
  void *va;
  uint pa_start;
  uint pa_end;
  int perm;
};

struct taskstate {
  uint link;         // Old ts selector
  uint esp0;         // Stack pointers and segment selectors
  ushort ss0;        //   after an increase in privilege level
  ushort padding1;
  uint *esp1;
  ushort ss1;
  ushort padding2;
  uint *esp2;
  ushort ss2;
  ushort padding3;
  void *cr3;         // Page directory base
  uint *eip;         // Saved state from last task switch
  uint eflags;
  uint eax;          // More saved state (registers)
  uint ecx;
  uint edx;
  uint ebx;
  uint *esp;
  uint *ebp;
  uint esi;
  uint edi;
  ushort es;         // Even more saved state (segment selectors)
  ushort padding4;
  ushort cs;
  ushort padding5;
  ushort ss;
  ushort padding6;
  ushort ds;
  ushort padding7;
  ushort fs;
  ushort padding8;
  ushort gs;
  ushort padding9;
  ushort ldt;
  ushort padding10;
  ushort t;          // Trap on task switch
  ushort iomb;       // I/O map base address
};

// Normal segment
#define SEG(type, base, lim, dpl) (struct gdt_desc)    \
{ ((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,      \
  ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uint)(lim) >> 28, 0, 0, 1, 1, (uint)(base) >> 24 }
#define SEG16(type, base, lim, dpl) (struct gdt_desc)  \
{ (lim) & 0xffff, (uint)(base) & 0xffff,              \
  ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uint)(lim) >> 16, 0, 0, 1, 0, (uint)(base) >> 24 }

void init_gdt();
void init_kvm();
pde_t *set_kvm();
void swtch_kvm();
int init_uvm(pde_t *pdir, char *start, int size);
pde_t *cp_uvm(pde_t *pgdir, int mem_size);
void free_uvm(pde_t *pgdir);
uint resize_uvm(pde_t *pgdir, uint oldsz, uint newsz);
void copy_user_page(uint dst, uint src);
void copy_to_user(uint dst, int off, char *src, int len);
void kmap_atomic(char *va, uint mem);
void unkmap_atomic(char *va);

struct proc;
void swtch_uvm(struct proc *p);
void map_page(pde_t *pdir, void *va, uint la, uint size, int perm);
uint unmap(pde_t *pdir, void *va);
pte_t *get_pte(pde_t *pdir, void *va, bool bcreate);

struct inode;
int load_uvm(pde_t *pdir, struct inode *ip, char *va, int off, int len);
void clear_pte(pde_t *pdir, void *va);
int copy_out(pde_t *pdir, char *dst, char *src, int len);
void do_page_fault();

#endif

