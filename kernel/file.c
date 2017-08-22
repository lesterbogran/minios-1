#include "kernel.h"

extern struct CPU cpu;

struct devrw devrw[MAX_DEV];
struct file file_table[NFILE];

int register_devrw(int dev, rw_func read, rw_func write)
{
	devrw[dev].read = read;
	devrw[dev].write = write;
	return 0;
}

void init_file() 
{
	int i;
	for(i = 0; i < NFILE; i++) {
		file_table[i].ref = 0;
	}
}

struct file *get_file(int fd)
{
	struct file *f;
	if(fd > NOFILE || fd < 0 || (f = cpu.cur_proc->ofile[fd]) == NULL) {
		return NULL;
	}
	return f;
}

int file_read(struct file *f, char *dst, int len)
{
	int ret;
	load_inode(f->ip);
	if(f->ip->de.type == T_DEV) {
		return devrw[f->ip->de.major].read(f->ip, dst, len);
	}
	ret = readi(f->ip, dst, f->off, len);
	f->off += ret;

	return ret;
}

int file_write(struct file *f, char *src, int len)
{
	int ret;
	load_inode(f->ip);
	if(f->ip->de.type == T_DEV) {
		return devrw[f->ip->de.major].write(f->ip, src, len);
	}
	
	ret = writei(f->ip, src, f->off, len);
	f->off += ret;

	return ret;
}

struct file *file_dup(struct file *f)
{
	if(f->ref <= 0)
		panic("file_dup\n");
	f->ref++;
	return f;
}

struct file *file_alloc()
{
	int i;
	for(i = 0; i < NFILE; i++) {
		if(file_table[i].ref == 0) {
			file_table[i].ref = 1;
			return file_table + i;
		}
	}
	panic("file_table max\n");
	return NULL;
}

int fd_alloc(struct file *f)
{
	int i;
	for(i = 0; i < NOFILE; i++){
		if(cpu.cur_proc->ofile[i] == NULL) {
			cpu.cur_proc->ofile[i] = f;
			return i;
		}
	}
	return -1;
}

struct inode *file_create(char *path, int type, int major, int minor)
{
	struct inode *dp, *ip;
	char name[DIR_NM_SZ];
	dp = namep(path, name);
	if(!dp) {
		return NULL;
	}
	load_inode(dp);
	if(dp->de.type != T_DIR) {
		irelese(dp);
		return NULL;
	}

	ip = dirlookup(dp, name);
	if(ip) {
		irelese(dp);
		load_inode(ip);
		if(type == T_FILE && ip->de.type == T_FILE)
			return ip;
		irelese(ip);
		return NULL;
	}

	ip = ialloc(dp->dev, type);
	if(!ip)
		panic("create file alloc inode error\n");

	load_inode(ip);
	ip->de.major = major;
	ip->de.minor = minor;
	ip->de.size = 0;
	ip->de.nlink = 1;
	iupdate(ip);

	if(type == T_DIR){
		dp->de.nlink++;
		iupdate(dp);
		dir_link(ip, ".", ip->inum);
		dir_link(ip, "..", dp->inum);
	}

	dir_link(dp, name, ip->inum);
	irelese(dp);

	return ip;
}

int file_mknod(char *path, int major, int minor) 
{
	struct inode *ip = file_create(path, T_DEV, major, minor);
	if(!ip)
		return -1;
	irelese(ip);
	return 0;
}

int file_mkdir(char *path, int major, int minor)
{
	struct inode *ip = file_create(path, T_DIR, major, minor);
	if(!ip)
		return -1;
	irelese(ip);
	return 0;
}

int file_open(char *path, int mode)
{
	int fd;
	struct file *f;
	struct inode *ip;
	if(mode & O_CREATE) {
		ip = file_create(path, T_FILE, 0, 0);
	}
	else {
		ip = namei(path);
		if(!ip) {
			return -1;
		}
		load_inode(ip);
		if(ip->de.type == T_DIR && mode > 0) {
			irelese(ip);
			return -2;
		}
	}

	f = file_alloc();
	f->type = FD_INODE;
	f->ip = ip;
	f->off = 0;

	fd = fd_alloc(f);
	if(fd < 0){
		file_close(f);
		return -3;
	}
	return fd;
}

int file_close(struct file *f)
{
	if(--f->ref == 0) {
		if(f->type == FD_INODE)
			irelese(f->ip);
		f->type = FD_NONE;
	}
	return 0;
}
