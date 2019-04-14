/*type.h*/

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

#define BLKSIZE  1024
#define NMINODE   100
#define NFD        16
#define NPROC       2

#define clear() printf("\033[H\033[J")

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;

char *cmdMenu[100] = { "ls", "cd", "pwd", "mkdir", "rmdir", "creat", "rm", "touch", "link", "symlink", "unlink", "chmod", "stat", "quit" };
char *disk = "mydisk";
char line[128], cmd[256], pathname[256], parameter[256];
char buf[BLKSIZE];
int my_fd, iblk;

typedef struct minode{
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

typedef struct mntable{
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

int fd, dev;
int nblocks, ninodes, bmap, imap, iblock;
int blk, offset;

int n, hold_fd;
char *name[256][256];

#endif
