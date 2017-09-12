#ifndef __FS_H__
#define __FS_H__

#define BLOCK_BUF_NUM 30
#define BLOCK_SIZE 512
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk
#define NDIRECT 12
#define NINDIRECT (BLOCK_SIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
#define INODE_NUM 128
#define SUPER_BLOCK_NUM 64
#define FILE_SYSTEM_NUM 64
#define I_VALID 0x2

#define IPER (BLOCK_SIZE / sizeof(struct dinode))
#define IBLOCK(inum,sb) ((inum) / IPER + (sb)->inodestart)
#define BPER (BLOCK_SIZE * 8)
#define BBLOCK(bnum,sb) ((bnum) / BPER + (sb)->bmapstart)

#define T_DIR 1
#define T_FILE 2
#define T_DEV 3
#define DIR_NM_SZ 14
#define ROOT_DEV 1
#define ROOT_INO 1

#include "type.h"

struct block_buf {
	int flags;
	int dev;
	int sec;
	int ref;
	struct block_buf *prev;
	struct block_buf *next;
	struct block_buf *qnext;
	char data[BLOCK_SIZE];
};

struct dinode {
	short type;           // File type
  	short major;          // Major device number (T_DEV only)
  	short minor;          // Minor device number (T_DEV only)
  	short nlink;          // Number of links to inode in file system
  	uint size;            // Size of file (bytes)
	uint ctime;
	uint mtime;
	uint atime;
  	uint addrs[NDIRECT+14];   // Data block addresses
};

struct file_system {
	int reg;
	char name[64];
	struct super_block_operation *s_op;
};

struct inode_operation {
	struct inode *(*dir_lookup)(struct inode *ip, char *name, int *off);
	void (*itrunc)(struct inode *ip);
	void (*dir_link)(struct inode *dp, char *name, int inum);
	struct inode *(*create)(struct inode *dp, char *name, int type, int major, int minor);
	int (*unlink)(struct inode *dp, char *name);
	int (*readi)(struct inode *ip, char *dst, int offset, int num);
	int (*writei)(struct inode *ip, char *src, int offset, int num);
	
};

struct inode {
  	uint dev;           // Device number
  	uint inum;          // Inode number
  	int ref;            // Reference count
  	int flags;          // I_VALID
	struct inode_operation *i_op;
	struct super_block *sb;

	struct dinode de;
};

struct super_block;
struct super_block_operation {
	void (*read_sb)(struct super_block *sb, int dev);
	struct inode *(*ialloc)(struct super_block *sb, int type);
	void (*read_inode)(struct super_block *sb, struct inode *ip);
	void (*write_inode)(struct super_block *sb, struct inode *ip);
};

struct file_operation;
struct super_block {
	uint size;         // Size of file system image (blocks)
  	uint nblocks;      // Number of data blocks
  	uint ninodes;      // Number of inodes.
  	uint nlog;         // Number of log blocks
  	uint logstart;     // Block number of first log block
  	uint inodestart;   // Block number of first inode block
  	uint bmapstart;    // Block number of first free map block
	int dev;
	struct super_block_operation *s_op;
	struct inode_operation *i_op;
	struct file_operation *f_op;
	struct inode *root;
};

struct dirent {
	short inum;
	char name[DIR_NM_SZ];
};

struct block_buf *bread(int dev, int sec);
void bwrite(struct block_buf *buf);
struct block_buf *bget(int dev, int sec);
void brelse(struct block_buf *buf);

void init_fs();
void mount_root();
struct super_block *get_sb(int dev);
struct inode *iget(int dev, int inum);
struct inode *idup(struct inode *ip);
void iput(struct inode *ip);

struct inode *namex(char *path, char *name, bool bparent);
struct inode *namei(char *path);
struct inode *namep(char *path, char *name);

#endif
