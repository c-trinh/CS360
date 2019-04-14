#ifndef _IGET_IPUT_GETINO_C
#define _IGET_IPUT_GETINO_C
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

char line[128], cmd[256], pathname[256], parameter[256];
char buf[BLKSIZE];               
int my_fd, iblk;
 
int tokenize(char *strtoTok)
{
	char *token;

	n = 0;
	token = strtok(strtoTok, "/");

	while (token != NULL)
	{
		strcpy(name[n], token);
		n++;
		token = strtok(NULL, "/");
	}
}

int search(MINODE *mip, char *name)
{
	char *cp;
	char dbuf[1024], sbuf[1024];


	for (int i = 0; i < 12; i++)
	{
		get_block(mip->dev, mip->INODE.i_block[i], dbuf);
		dp = (DIR *)dbuf;
		cp = dbuf;

		if (mip->INODE.i_block[i] == 0)
			return 0;

		while(cp < &dbuf[1024])
		{
			strncpy(sbuf, dp->name, dp->name_len);
			sbuf[dp->name_len] = 0;


			if (strcmp(sbuf, name) == 0)
			{
				printf("The directory is found at INODE: %d!\n", dp->inode);
				return dp->inode;
			} 

			//else continue
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	printf("Directory not found...\n");
	return 0;
}

MINODE *iget(int dev, int ino)
{
  int i, blk, disp;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;

  for (i=0; i < NMINODE; i++){
	mip = &minode[i];
	if (mip->dev == dev && mip->ino == ino){
	   mip->refCount++;	
	   //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
	   return mip;
	}
  }
  for (i=0; i < NMINODE; i++){
	mip = &minode[i];
	if (mip->refCount == 0){
	   mip->refCount = 1;
	   mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
	   mip->dirty = mip->mounted = mip->mptr = 0;
	   // get INODE of ino into buf[ ]      
	   blk  = (ino-1)/8 + iblock;  // iblock = Inodes start block #
	   disp = (ino-1) % 8;
	   get_block(dev, blk, buf);
	   ip = (INODE *)buf + disp;
	   // copy INODE to mp->INODE
	   mip->INODE = *ip;
	   return mip;
	}
  }   
  printf("no more\n");
  return 0;
}

int iput(MINODE *mip)
{
	int offset, block;
	char buff[1024];

	mip->refCount--;

	if (mip->refCount > 0){
		 return; 
		}
	if (!mip->dirty) {
		 return;
		}

	printf("iput: dev = %d ino = %d\n", mip->dev, mip->ino);

	block = (mip->ino - 1)/8 + iblock;
	offset = (mip->ino - 1)%8;

	get_block(mip->dev, block, buff);

	ip = (INODE *)buff + offset;
	*ip = mip->INODE;

	put_block(mip->dev, block, buff);
}

int getino(int *dev, char *pathname)
{

  	int i, ino, blk, disp;
  	char buff[BLKSIZE];
  	INODE *ip;
  	MINODE *mip;
	 
	if (strcmp(pathname, "/")==0) //checks root
	  	return 2;
	 
  	if (pathname[0]=='/') //if absolute go get root minode
  	{
	 	mip = iget(*dev, 2);
  	}
  	else //else go get running minode
  	{
  		 
  		mip = iget(running->cwd->dev, running->cwd->ino);
  	}

  	 
	strcpy(buff, pathname);

	//Tokenize the pathname, remove '/' and count.
	tokenize(buff); // n = number of token strings

	//n is how long the pathname is
	for (i=0; i < n; i++){
		printf("===========================================\n");
		printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
	 	
	 	//Search for that tok pathname, return the ino if found
		ino = search(mip, name[i]);
		//if the name doesn't exist, return 0.
		if (ino==0){
			iput(mip);
			printf("name %s does not exist\n", name[i]);
			return 0;
		}
		//Release that minode.
		iput(mip);
		mip = iget(*dev, ino);
   	}
   	return ino;
}

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}
int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
	 return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}
#endif
