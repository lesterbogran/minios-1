#include "kernel.h"

static bool name_is_digital(char *name)
{
	while(*name) {
		if(!isdigit(*name))
			return false;
		name++;
	}
	return true;
}

struct inode *proc_root_dir_lookup(struct inode *dp, char *name, int *off)
{
	struct inode *ip;
	if(!strcmp(name, "sysinfo")) {
		ip = iget(dp->sb, 1);
	}
	else if(name_is_digital(name)){
		int pid = atoi(name);
		struct proc *p = get_proc(pid);
		if(!p)
			ip = NULL;
		else
			ip = iget(dp->sb, pid << 16 | 1);
	}
	else 
		ip = NULL;
	return ip;
}

int proc_root_readdir(struct inode *dp, struct dirent *de)
{
	extern struct proc proc_table[MAX_PROC];
	int i;
	int num = 0;
	for(i = 0; i < MAX_PROC; i++) {
		if(proc_table[i].stat != UNUSED) {
			sprintf((de + num)->name, "%d", proc_table[i].pid);
			num++;
		}
	}
	sprintf((de + num)->name, "sysinfo");
	num++;
	return num;
}

struct inode_operation proc_root_inode_op = {
	.dir_lookup = proc_root_dir_lookup,
	.readdir = proc_root_readdir,
};

int proc_sysinfo_readi(struct inode *ip, char *dst, int offset, int num)
{
	int wd = 0;
	extern uint _version;
	extern uint user_ticks, kern_ticks, ticks;
	wd += sprintf(dst, "%d %d %d %d %d %d\n", 
						_version,
						PHYSICAL_END, 
						size_of_free_memory(),
						user_ticks,
						kern_ticks,
						ticks
				);
	return wd;
}

struct inode_operation proc_sysinfo_inode_op = {
	.readi = &proc_sysinfo_readi,
};

int proc_proc_readi(struct inode *ip, char *dst, int offset, int num)
{
	if(ip->type == T_DIR)
		return -1;
	int rd = 0;
	int pid = ip->inum >> 16;
	int low = ip->inum & 0xffff;
	struct proc *p = get_proc(pid);
	if(!p)
		return -1;
	if(low == 2) {
		rd += sprintf(dst, "%d %d %d %d %d %d\n", 
				p->pid, 
				p->parent ? p->parent->pid : 0,
				p->vend - USER_LINK,
				p->stat,
				p->ticks,
				p->priority
			);
	}
	else if(low == 3){
		rd += sprintf(dst, p->name);
	}
	return rd;
}

struct inode *proc_proc_dirlookup(struct inode *dp, char *name, int *off)
{
	int pid = dp->inum >> 16;
	int low = dp->inum & 0xffff;
	if(low != 1)
		return NULL;
	if(!strcmp(name, "stat")) {
		return iget(dp->sb, pid << 16 | 2);
	}
	else if(!strcmp(name, "cmdline")) {
		return iget(dp->sb, pid << 16 | 3);
	}
	return NULL;
}

int proc_proc_readdir(struct inode *dp, struct dirent *de)
{
	int num = 0;
	if(dp->type != T_DIR)
		return -1;
	sprintf((de + num)->name, "stat");
	num++;
	sprintf((de + num)->name, "cmdline");
	num++;
	return num;
}

struct inode_operation proc_proc_inode_op = {
	.readi = proc_proc_readi,
	.dir_lookup = proc_proc_dirlookup,
	.readdir = proc_proc_readdir,
};

