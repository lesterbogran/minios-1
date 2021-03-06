#include "kernel.h"

struct proc proc_table[MAX_PROC];
extern struct CPU cpu;
struct proc *init_proc;

uint next_pid = 0;

void swtch(struct context **old, struct context *new);

void trapret();

void forkret()
{
	static bool first = true;
	if(first) {
		first = false;
		mount_root();
		mount_fs("/proc", "proc");
	}
	else {
		popsti();
	}
}

struct proc *alloc_proc()
{
	struct proc *p = NULL;
	int i = 0;
	for(;i < MAX_PROC; i++) {
		p = proc_table + i;
		if(p->stat == UNUSED)
			break;
		p = NULL;
	}
	if(!p)
		return NULL;
	p->kstack = (uint)kalloc();
	if(!p->kstack) {
		p->stat = UNUSED;
		return NULL;
	}
	p->pid = ++next_pid;
	p->stat = EMBRYO;
	p->killed = 0;
	for(i = 0; i < NOFILE; i++) {
		p->ofile[i] = NULL;
	}
	uint sp = p->kstack + PG_SIZE;
	sp -= sizeof(struct trap_frame);
	p->tf = (struct trap_frame *)sp;
	sp -= 4;
	*(uint *)sp = (uint)trapret;
	sp -= sizeof(struct context);
	p->context = (struct context *)sp;
	p->context->eip = (uint)forkret;
	p->ticks = 0;
	p->signal = 0;
	for(i = 0; i < 32; i++) {
		p->sig_handlers[i].handler = NULL;
	}
	p->priority = DEF_PRIORITY;
	return p;
}

void scheduler()
{
	struct proc *p = NULL, *next = NULL;
	int i;
	int count;
	while(1) {
		sti();
		count = 0;
		for(i = 0; i < MAX_PROC; i++) {
			p = proc_table + i;
			if(p->stat == RUNNING && p->count > count) {
				count = p->count;
				next = p;
			}
		}
		if(count == 0) {
			for(i = 0; i < MAX_PROC; i++) {
				p = proc_table + i;
				if(p->stat == RUNNING || p->stat == SLEEPING) {
					p->count = p->count / 2 + p->priority;
				}
			}
			continue;
		}

		cpu.cur_proc = next;
		swtch_uvm(next);
		swtch(&cpu.context, next->context);
		swtch_kvm();
		cpu.cur_proc = NULL;
	}
}

void user_init()
{
	extern char _binary__obj_initcode_start[], _binary__obj_initcode_size[];
	struct proc *p = alloc_proc();
	if(!p)
		panic("user_init error\n");
	p->pgdir = set_kvm();
	p->vend = PG_SIZE;
	init_uvm(p->pgdir, _binary__obj_initcode_start, (int)_binary__obj_initcode_size);
	
	memset(p->tf, 0, sizeof(*p->tf));

	init_proc = p;
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  	p->tf->es = p->tf->ds;
  	p->tf->ss = p->tf->ds;
  	p->tf->eflags = FL_IF;
  	p->tf->esp = PG_SIZE;
  	p->tf->eip = 0;

	p->parent = NULL;
	p->stat = RUNNING;
}

int fork() 
{
	int i;
	struct proc *p = alloc_proc();
	if(!p)
		return -1;
	p->pgdir = cp_uvm(cpu.cur_proc->pgdir, cpu.cur_proc->vend);
	if(!p->pgdir) {
		p->stat = UNUSED;
		return -1;
	}
	for(i = 0; i < NOFILE; i++) {
		if(cpu.cur_proc->ofile[i])
			p->ofile[i] = file_dup(cpu.cur_proc->ofile[i]);
	}
	*p->tf = *cpu.cur_proc->tf;
	p->tf->eax = 0;
	p->parent = cpu.cur_proc;
	p->vend = cpu.cur_proc->vend;
	p->cwd = ddup(cpu.cur_proc->cwd);
	p->exe = ddup(cpu.cur_proc->exe);
	p->stat = RUNNING;
	p->priority = cpu.cur_proc->priority;

	return p->pid;
}

void sched()
{
	if(cpu.ncli != 1)
		panic("pid:%d sched ncli:%d\n", cpu.cur_proc->pid, cpu.ncli);
	if(read_eflags() & FL_IF)
		panic("pid:%d sched FL_IF\n", cpu.cur_proc->pid);
	swtch(&cpu.cur_proc->context, cpu.context);
}

void yield()
{
	pushcli();
	sched();
	popsti();
}

void sleep_on(void *chan)
{
	pushcli();
	cpu.cur_proc->sleep_chan = chan;
	cpu.cur_proc->stat = SLEEPING;
	sched();

	cpu.cur_proc->sleep_chan = NULL;
	popsti();
}

void wakeup_on(void *chan)
{
	int i = 0;
	for(; i < MAX_PROC; i++) {
		struct proc *p = proc_table + i;
		if(p->stat == SLEEPING && p->sleep_chan == chan)
			p->stat = RUNNING;
	}
}

void sleep()
{
	pushcli();
	cpu.cur_proc->stat = SLEEPING;
	sched();
	popsti();
}

void wakeup(struct proc *p)
{
	if(p->stat != SLEEPING || p->sleep_chan) {
		return;
	}
	p->stat = RUNNING;
}

int execv(char *path, char *argv[], char *envp[])
{
	struct dentry *de = namei(path);
	if(!de) {
		return -1;
	}

	if(de->ip->type == T_DIR) {
		dput(de);
		return -2;
	}
	struct elfhdr elf;
	de->ip->i_op->readi(de->ip, (char *)&elf, 0, sizeof(struct elfhdr));
	if(elf.magic != ELF_MAGIC) {
		//err_info("elf magic error: %p\n", elf.magic);
		dput(de);
		return -3;
	}

	pde_t *pdir = set_kvm();
	if(!pdir)
		panic("exec set_kvm\n");

	struct proghdr ph;
	uint size = USER_LINK;
	uint off = elf.phoff;
	int num = 0;
	while(num++ < elf.phnum) {
		de->ip->i_op->readi(de->ip, (char *)&ph, off, sizeof(struct proghdr));
		off += sizeof(struct proghdr);
		if(ph.type != ELF_PROG_LOAD)
			continue;
		size = resize_uvm(pdir, size, ph.va + ph.memsz);
		load_uvm(pdir, de->ip, (char *)ph.va, ph.offset, ph.filesz);
	}
	dput(cpu.cur_proc->exe);
	cpu.cur_proc->exe = de;
	size = PG_ROUNDUP(size);
	size = resize_uvm(pdir, size, size + USTACKSIZE + PG_SIZE);
	clear_pte(pdir, (char *)(size - (USTACKSIZE + PG_SIZE)));

	uint sp = size;
	uint ustack[10];
	int argc;
	int sz = 0;
	for(argc = 0; argv[argc]; argc++) {
		sz = strlen(argv[argc]) + 1;
		sp -= sz;
		if(copy_out(pdir, (char *)sp, argv[argc], sz) < 0)
			panic("copy ustack error\n");
		ustack[argc + 2] = sp;
	}
	ustack[argc + 2] = 0;

	ustack[0] = argc;
	ustack[1] = sp - argc * 4;

	sp -= (argc * 4 + 8);
	copy_out(pdir, (char *)sp, (char *)ustack, argc * 4 + 8);

	pde_t *old = cpu.cur_proc->pgdir;
	cpu.cur_proc->pgdir = pdir;
	cpu.cur_proc->vend = size;
	cpu.cur_proc->tf->esp = sp;
	cpu.cur_proc->tf->eip = elf.entry;
	memset(cpu.cur_proc->cmdline, 0, PROC_CMD_SZ);

	int i;
	int str_len = 0, tot_len = 0;
	for(i = 0; i < argc; i++) {
		str_len = strlen(argv[i]);
		memmove(cpu.cur_proc->cmdline+tot_len, argv[i], str_len);
		tot_len += str_len;
		memmove(cpu.cur_proc->cmdline+tot_len, " ", 1);
		tot_len += 1;
	}

	swtch_uvm(cpu.cur_proc);

	free_uvm(old);
	return 0;
}

void exit() 
{
	struct proc *p;
	int i;
	if(cpu.cur_proc == init_proc)
		panic("init exit!!\n");
	cpu.cur_proc->killed = 1;
	
	for(i = 0; i < NOFILE; i++) {
		if(cpu.cur_proc->ofile[i]) {
			file_close(cpu.cur_proc->ofile[i]);
			cpu.cur_proc->ofile[i] = NULL;
		}
	}
	dput(cpu.cur_proc->cwd);
	dput(cpu.cur_proc->exe);
	cpu.cur_proc->cwd = NULL;
	cpu.cur_proc->exe = NULL;

	pushcli();

	for(i = 0; i < MAX_PROC; i++) {
		p = proc_table + i;
		if(p->parent == cpu.cur_proc)
			p->parent = init_proc;
			if(p->stat == ZOMBIE)
				kill(1, SIG_CHLD);
	}
	
	cpu.cur_proc->stat = ZOMBIE;
	kill(cpu.cur_proc->parent->pid, SIG_CHLD);
	
	sched();
	panic("exit zombie\n");
}

int wait()
{
	int pid;
	struct proc *p;
	int i;
	while(1) {
		for(i = 0; i < MAX_PROC; i++) {
			p = proc_table + i;
			if(p->stat == ZOMBIE && p->parent == cpu.cur_proc) {
				pid = p->pid;
				kfree((char *)p->kstack);
				p->kstack = NULL;
				free_uvm(p->pgdir);
				p->pid = 0;
				p->parent = NULL;
				p->stat = UNUSED;
				memset(p->cmdline, 0, PROC_CMD_SZ);
				return pid;
			}
		}
		sleep();
		if(cpu.cur_proc->signal & ~SIGNAL(SIG_CHLD))
			return 0;
	}
	return 0;
}

struct proc *get_proc(int pid)
{
	int i;
	for(i = 0; i < MAX_PROC; i++) {
		if(proc_table[i].pid == pid)
			return proc_table + i;
	}
	return NULL;
}

