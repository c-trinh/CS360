#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "assist.c"
#include "sys_message.c"

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	write(fd, buf, BLKSIZE);
}

int enter_name(MINODE *pip, int myino, char *myname)
{
	char buff[BLKSIZE], buff2[BLKSIZE];
	char *cp;
	int checker=0;
	int i;

	for (i = 0; i < 13; i++)
	{
		if (pip->INODE.i_block[i] == 0)
		{
			break;
		}
		get_block(pip->dev, pip->INODE.i_block[i], buff);

		dp = (DIR *)buff;
		cp = dp;


		while (cp + dp->rec_len < buff + BLKSIZE)
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		int needLen = (4*((8+dp->name_len+3)/4));
		int remain = (dp->rec_len) - needLen;

		if (remain >= needLen)
		{
			dp->rec_len = needLen;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			dp->inode = myino;
			dp->rec_len = remain;
			dp->name_len = strlen(myname);
			strcpy(dp->name, myname);

			put_block(pip->dev, pip->INODE.i_block[i], buff);
			checker = 1;
		}
	}
	if (checker == 0)
	{
		pip->INODE.i_block[i] = balloc(dev);
		pip->INODE.i_size += BLKSIZE;
		pip->INODE.i_blocks += 2;

		memset(buff, 0, BLKSIZE);
		get_block(pip->dev, pip->INODE.i_block[i], buff);
		dp = (DIR *)buff;
		cp = buff;
		dp->rec_len = BLKSIZE;
		dp->name_len = strlen(myname);
		dp->inode = myino;
		strcpy(dp->name, myname);
		put_block(pip->dev, pip->INODE.i_block[i], buff);
	}
}

/*Returns the inode number of a pathname*/
int getino(int *dev, char *pathname)
{
	int i, ino, blk, disp;
	char buff[BLKSIZE];
	INODE *ip;
	MINODE *mip;

	if (strcmp(pathname, "/") == 0) //checks root
		return 2;

	if (pathname[0] == '/') //if absolute go get root minode
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
	for (i = 0; i < n; i++) {
		printf("----\n");
		printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
		printf("----\n\n");

		//Search for that tok pathname, return the ino if found
		ino = search(mip, name[i]);
		//if the name doesn't exist, return 0.
		if (ino == 0) {
			iput(mip);
			printf("[!]-\"Pathname \'%s\' not found in CWD.\"\n", name[i]);
			return 0;
		}
		//Release that minode.
		iput(mip);
		mip = iget(*dev, ino);
	}
	return ino;
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int balloc(int dev)
{
  int i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
      set_bit(buf,i);
      put_block(dev, bmap, buf);
      decFreeBlocks(dev);

      printf("Block allocated:%d\n", i+1);

      return i+1;
    }
  }
  printf("No more free inodes\n");
  return 0;
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("No more free inodes\n");
  return 0;
}

int mymkdir(MINODE *pip, char *name)
{
	int tempIno, tempBno;
	MINODE *mip;
	char buff[BLKSIZE];
	char *cp;

	tempIno = ialloc(dev);
	tempBno = balloc(dev);

	mip = iget(dev, tempIno);

	//Temporary inode fill out new memory slot.
	INODE *tip = &mip->INODE;

	tip->i_mode = 0x41ED;
	tip->i_uid = running->uid;
	tip->i_gid = 0;
	tip->i_size = BLKSIZE;
	tip->i_links_count = 2;
	tip->i_atime = tip->i_ctime = tip->i_mtime = time(0L);
	tip->i_blocks = 2;
	tip->i_block[0] = tempBno;

	for (int i = 1; i < 15; i++)
	{
		tip->i_block[i] = 0;
	}

	mip->dirty = 1;
	iput(mip);

	get_block(dev, tip->i_block[0], buff);

	dp = (DIR *)buff;
	cp = dp;

	dp->inode = tempIno;
	printf("tempino: %d\n", tempIno);
	dp->rec_len = 12;
	dp->name_len = 1;
	strcpy(dp->name, ".");

	cp += dp->rec_len;
	dp = (DIR *)cp;

	dp->inode = pip->ino;
	printf("pip->ino: %d\n",pip->ino);
	dp->rec_len = BLKSIZE-12;
	dp->name_len = 2;
	strcpy(dp->name, "..");

	put_block(dev, tempBno, buff);
	enter_name(pip, tempIno, name);
}

int incFreeBlocks(int dev)
{
  char buff[BLKSIZE];

  get_block(dev, 1, buff);
  sp = (SUPER *)buff;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buff);

  get_block(dev, 2, buff);
  gp = (GD *)buff;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buff);

}

int bdealloc(int dev, int bno)
{
	char buff[BLKSIZE];
	//Grab the bmap block
	get_block(dev, bmap, buff);
	//Clr the bit we would like
	clr_bit(buff, bno);
	//Increase the super and gd count
	incFreeBlocks(dev);
	//put it back not use anymore.
	put_block(dev, bmap, buff);
}

int incFreeInodes(int dev)
{
  char buff[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buff);
  sp = (SUPER *)buff;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buff);

  get_block(dev, 2, buff);
  gp = (GD *)buff;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buff);
}

int idealloc(int dev, int ino)
{
	char buff[BLKSIZE];

	get_block(dev, imap, buff); //in ino instead
	clr_bit(buff, ino);
	incFreeInodes(dev);
	put_block(dev, imap, buff);
}


int truncate(MINODE *mip)
{
	for (int i = 0; i < 12; i++)
	{
		if (mip->INODE.i_block[i] == 0)
			break;
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	mip->dirty = 1;
	mip->INODE.i_size = 0;
}
