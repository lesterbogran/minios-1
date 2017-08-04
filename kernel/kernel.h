#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "type.h"
#include "x86.h"
#include "cga.h"
#include "crt.h"
#include "memory.h"
#include "vm.h"
#include "trap.h"
#include "pic.h"
#include "uart.h"
#include "mmu.h"
#include "timer.h"

#define LOG cprintf
#define sys_info cprintf
#define printf cprintf

#endif

