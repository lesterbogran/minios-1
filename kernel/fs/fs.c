#include "kernel.h"

extern struct CPU cpu;
struct super_block *root_sb;

struct super_block super_blocks[SUPER_BLOCK_NUM];
struct file_system file_systems[FILE_SYSTEM_NUM];
void init_file();
void binit();
void register_file_system_minios(struct file_system *fs);
void register_file_system_proc(struct file_system *fs);

struct super_block *get_sb(int dev)
{
	int i;
	for(i = 0; i < SUPER_BLOCK_NUM; i++) {
		if(super_blocks[i].dev == dev)
			return &super_blocks[i];
	}
	if(dev == 0)
		panic("super_blocks not enough\n");
	return NULL;
}

struct file_system *get_fs_type(char *fs_name)
{
	int i;
	for(i = 0; i < FILE_SYSTEM_NUM; i++) {
		if(!strcmp(file_systems[i].name, fs_name))
			return &file_systems[i];
	}
	return NULL;
}

struct file_system *get_free_fs()
{
	int i;
	for(i = 0; i < FILE_SYSTEM_NUM; i++) {
		if(file_systems[i].reg == 0)
			return &file_systems[i];
	}
	panic("get_free_fs\n");
}

void register_file_system()
{
	struct file_system *fs;
	fs = get_free_fs();
	register_file_system_minios(fs);
	fs = get_free_fs();
	register_file_system_proc(fs);
}

void init_fs()
{
	int i;
	for(i = 0; i < SUPER_BLOCK_NUM; i++) {
		super_blocks[i].dev = 0;
	}
	for(i = 0; i < FILE_SYSTEM_NUM; i++) {
		file_systems[i].reg = 0;
	}
	binit();
	init_file();
	register_file_system();
}

void mount_root()
{
	struct file_system *fs = get_fs_type("minios");
	if(!fs)
		panic("can't find file_system minios\n");
	struct super_block *sb = get_sb(0);
	sb->dev = ROOT_DEV;
	fs->s_op->read_sb(sb);
	root_sb = sb;
	sb->root = namei("/");
	sb->root->sb = sb;
	sb->root->i_op = sb->i_op;
	sb->s_op->read_inode(sb, sb->root);
	cpu.cur_proc->cwd = idup(sb->root);

	printf("mount root file system 'minios' success\n");
}

int mount_fs(char *path, char *fs_name)
{
	static int dev = ROOT_DEV + 1;

	struct inode *dp = namei(path);
	if(!dp) {
		printf("%s not exists\n", path);
		return -1;
	}
	if(dp->type != T_DIR) {
		printf("%s is not direct\n", path);
		return -1;
	}
	struct file_system *fs = get_fs_type(fs_name);
	if(!fs) {
		printf("file_system '%s' may not register\n", fs_name);
		return -1;
	}
	struct super_block *sb = get_sb(0);
	sb->dev = dev++;
	fs->s_op->read_sb(sb);
	dp->mount = idup(sb->root);
	sb->cover = idup(dp);
	iput(dp);
	
	printf("mount file system '%s' success\n", fs_name);
	return 0;
}

char *path_decode(char *path, char *name)
{
	char *sub_path;
	if(*path == '/')
		path++;
	if(!*path)
		return NULL;
	sub_path = path;
	while(*path != '/' && *path)
		path++;
	int len = path - sub_path;
	if(name) {
		if(len >= DIR_NM_SZ) {
			memmove(name, sub_path, DIR_NM_SZ);
		}
		else {
			memmove(name, sub_path, len);
			name[len] = 0;
		}
	}
	while(*path == '/')
		path++;
	return path;
}

struct inode *namex(char *path, char *name, bool bparent)
{
	struct super_block *sb;
	struct inode *ip, *next;
	int off;
	if(*path == '/')
		ip = iget(root_sb, ROOT_INO);
	else
		ip = idup(cpu.cur_proc->cwd);

	while((path = path_decode(path, name)) != 0) {
		if(ip->type != T_DIR){
			err_info("namex ip dev:%d inum:%d type:%d isn't directory path:%s, name:%s\n", ip->dev, ip->inum, ip->type, path, name);
			iput(ip);
			return NULL;
		}
		if(bparent && *path == 0) {
			return ip;
		}
		if(!strcmp(name, "..")) {
			if((sb = ip->sb) && (ip == sb->root) && sb->cover) {
				iput(ip);
				ip = idup(sb->cover);
			}
		}
		next = ip->i_op->lookup(ip, name, &off);
		if(!next) {
			iput(ip);
			return NULL;
		}
		iput(ip);
		ip = next;
	}
	if(bparent) {
		iput(ip);
		return NULL;
	}
	return ip;
}

struct inode *namei(char *path)
{
	char name[DIR_NM_SZ];
	return namex(path, name, false);
}

struct inode *namep(char *path, char *name)
{
	return namex(path, name, true);
}

