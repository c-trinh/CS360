/************* cd_ls_pwd.c file **************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "ext2fs.h"
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include "type.h"

//MiNode = Memory iNode

extern MINODE minode[NMINODE];
extern MINODE *root;

extern PROC   proc[NPROC], *running;
extern MNTABLE mntable, *mntPtr;

extern SUPER *sp;
extern GD    *gp;
extern INODE *ip;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[64];

extern char gpath[128];   // hold tokenized strings
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings

extern MINODE minode[ ];
extern PROC *running;
extern int fd, dev;
extern int bmap, imap, iblk;


extern char gpath[128];   // hold tokenized strings
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings

extern MINODE *iget();

change_dir()
{
  char temp[256];
  char buf[BLKSIZE];
  DIR *dp;
  MINODE *ip, *newip, *cwd;
  int dev, ino;
  char c;

  if (pathname[0] == 0){
     iput(running->cwd);
     running->cwd = iget(root->dev, 2);
     return;
  }

  if (pathname[0] == '/')  dev = root->dev;
  else                     dev = running->cwd->dev;
  strcpy(temp, pathname);
  ino = getino(dev, temp);

  if (!ino){
     printf("cd : no such directory\n");
     return(-1);
  }
  printf("dev=%d ino=%d\n", dev, ino);
  newip = iget(dev, ino);    /* get inode of this ino */

  printf("mode=%4x   ", newip->INODE.i_mode);
  //if ( (newip->INODE.i_mode & 0040000) == 0){
  if (!S_ISDIR(newip->INODE.i_mode)){
     printf("%s is not a directory\n", pathname);
     iput(newip);
     return(-1);
  }

  iput(running->cwd);
  running->cwd = newip;

  printf("after cd : cwd = [%d %d]\n", running->cwd->dev, running->cwd->ino);
}

int ls_file(MINODE *mip, char *name)
{
  int k;
  u16 mode, mask;
  char mydate[32], *s, *cp, ss[32];

  mode = mip->INODE.i_mode;
  if (S_ISDIR(mode))
      putchar('d');
  else if (S_ISLNK(mode))
      putchar('l');
  else
      putchar('-');

   mask = 000400;
   for (k=0; k<3; k++){
      if (mode & mask)
         putchar('r');
      else
         putchar('-');
      mask = mask >> 1;

     if (mode & mask)
        putchar('w');
     else
        putchar('-');
        mask = mask >> 1;

     if (mode & mask)
        putchar('x');
     else
        putchar('-');
        mask = mask >> 1;

     }
     printf("%4d", mip->INODE.i_links_count);
     printf("%4d", mip->INODE.i_uid);
     printf("%4d", mip->INODE.i_gid);
     printf("  ");

     s = mydate;
     s = (char *)ctime(&mip->INODE.i_ctime);
     s = s + 4;
     strncpy(ss, s, 12);
     ss[12] = 0;

     printf("%s", ss);
     printf("%8ld",   mip->INODE.i_size);

     printf("    %s", name);

     if (S_ISLNK(mode))
        printf(" -> %s", (char *)mip->INODE.i_block);
     printf("\n");

}

int ls_dir(MINODE *mip)
{
  int i;
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *dip;

  for (i=0; i<12; i++){ /* search direct blocks only */
     printf("i_block[%d] = %d\n", i, mip->INODE.i_block[i]); //print the index of i_block array
     if (mip->INODE.i_block[i] == 0) //if empty return 0
         return 0;

     get_block(mip->dev, mip->INODE.i_block[i], sbuf); // if i_block is not empty get_block called, read into sbuf
     dp = (DIR *)sbuf; //cast to DIR * type
     cp = sbuf; //point current pointer to 1st index of sbuf

     while (cp < sbuf + BLKSIZE){ //while index of sbuf does not go out of range
        strncpy(temp, dp->name, dp->name_len); // copy from name at length name_len into temp
        temp[dp->name_len] = 0; //set last character to null pointer for string
        dip = iget(dev, dp->inode); //get  directory inode pointer from dp->inode
        ls_file(dip, temp); // call to ls_file to complete ls
        iput(dip); //put directory inode pointer

        cp += dp->rec_len; //increment cp by record length
        dp = (DIR *)cp; //set directory pointer to new directory pointer to ls on next loop
     }
  }
}

int list_file()
{	//Figures whether to call ls_dir or ls_file
  MINODE *mip;	//memory inode pointer
  u16 mode;
  int dev, ino;

  if (pathname[0] == 0)	//If not given a pathname value is 0 and calls ls_dir on cwd
    ls_dir(running->cwd);
  else{	//Calls ls_file
    dev = root->dev;	// set device to point at root's device
    ino = getino(dev, pathname); // calling getino on the device set in previous line and using pathname
    if (ino==0){ // if null inode doesn't exist
      printf("no such file %s\n", pathname);
      return -1;	//return
    }
    mip = iget(dev, ino); //inode in memory is set to the value in iget with device and ino
    mode = mip->INODE.i_mode; //set mode to inode in memory pointer's mode
    if (!S_ISDIR(mode)) //checks if mode is not dir
      ls_file(mip, (char *)basename(pathname)); //if isn't dir, ls_file
    else
      ls_dir(mip); // if is dir, ls_dir
    iput(mip); // puts blocks from minode pointer and sets the inode to that value
  }
}

int rpwd(MINODE *wd)
{
  char buf[BLKSIZE], myname[256], *cp;
  MINODE *parent, *ip;
  u32 myino, parentino;
  DIR   *dp;

  if (wd == root) //If cwd is still in root, end
      return;

  parentino = findino(wd, &myino);  //Finds inode that belongs to the directory
  parent = iget(dev, parentino);    //Return minode pointer to loaded INODE

  findmyname(parent, myino, myname); // ?
  // recursively call rpwd()
  rpwd(parent);

  iput(parent); // ?
  printf("/%s", myname);

  return 1;
}

char *pwd(MINODE *wd)
{
  if (wd == root){
    printf("/\n");
    return;
  }
  rpwd(wd);
  printf("\n");
}
