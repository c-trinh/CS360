#ifndef _TYPEH_
#define _TYPEH_
#include "ext2fs.h"
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sb;
GD    *gd;
INODE *ip;
DIR   *dp;

#define BLKSIZE  1024
#define NMINODE   100
#define NFD        16
#define NPROC       2


typedef struct minode{	//Memory inode
  INODE INODE;
  int dev, ino;	//Device
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mptr;
}MINODE;


typedef struct oft{ //open file tree
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;


typedef struct proc{ //Process
  struct proc *next;
  int          pid;
  int          uid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;


typedef struct mntable{	//mount table
  int dev;
  int ninodes;
  int nblocks;
  int bmap;
  int imap;
  int iblk;
  MINODE *mntDirPtr;
  char devName[64];
  char mntName[64];
}MNTABLE;

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblock;
extern int blk, offset;

extern int n, hold_fd;
extern char *name[256][256];



#endif
