/*********** util.c file ****************/
/*****************
int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   
****************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include "type.h"


extern MINODE minode[ ];
extern PROC *running;
extern int fd, dev;
extern int bmap, imap, iblk;


extern char gpath[128];   // hold tokenized strings
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings 

/*
char kcwnames[64][128],*kcwname[64]; 
int  kcwn;
*/

// return minode pointer to loaded INODE
MINODE *kcwiget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;
  
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       printf("found [%d %d] at minode[%d]: return its addr\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
      printf("load INODE=[%d %d] into minode[%d]\n", dev, ino, i);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk  = (ino-1)/8 + iblk;
       disp = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

kcwiput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write back */
 printf("KCWiput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 block =  ((mip->ino - 1) / 8) + iblk;
 offset =  (mip->ino - 1) % 8;

 /* first get the block containing this inode */
 get_block(mip->dev, block, buf);

 ip = (INODE *)buf + offset;
 *ip = mip->INODE;

 put_block(mip->dev, block, buf);

} 

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);
  strcpy(gpath, pathname);
  n = 0;

  s = strtok(gpath, "/");

  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }

  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

int kcwsearch(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE];
   DIR *dp;
   INODE *ip;

   printf("KCWsearch:search for %s in MINODE = [%d, %d]\n", 
           name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /**********  search for a file name ***************/
   for (i=0; i<12; i++){ /* search direct blocks only */
        printf("KCWsearch: i=%d  i_block[%d]=%d\n", i, i, ip->i_block[i]);
        if (ip->i_block[i] == 0)
           return 0;

	//getchar();

        get_block(dev, ip->i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        printf("   i_number rec_len name_len    name\n");

        while (cp < sbuf + BLKSIZE){
	    c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;
           
            printf("%8d%8d%8u        %s\n", 
                    dp->inode, dp->rec_len, dp->name_len, dp->name);
            if (strcmp(dp->name, name)==0){
                printf("found %s : ino = %d\n", name, dp->inode);
                return(dp->inode);
            }
            dp->name[dp->name_len] = c;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
   }
   return(0);
}

int kcwgetino(int dev, char *pathname)
{
  int i, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("KCWgetino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
    mip = kcwiget(dev, 2);
  else
    mip = kcwiget(running->cwd->dev, running->cwd->ino);

  strcpy(buf, pathname);
  tokenize(buf);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("KCWgetino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = kcwsearch(mip, name[i]);

      if (ino==0){
         kcwiput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      kcwiput(mip);
      mip = kcwiget(dev, ino);
   }
   return ino;
}
