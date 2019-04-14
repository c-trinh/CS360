/*util.c*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

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

int enter_name(MINODE *pmip, int myino, char *myname)
{
	//create a DIR entry
	char buff[BLKSIZE];
	char *cp;
	int flag	 = 0;
	int i;
	int need_len, remain;

	for (i = 0; i < 13; i++)
	{
		if (pmip->INODE.i_block[i] == 0)
		{
			//if block only has one entry enter new entry as first entry
			break;
		}
		//get DIR block that is put right before function call
		get_block(pmip->dev, pmip->INODE.i_block[i], buff);

		dp = (DIR *)buff;
		cp = dp;

		//going to last entry in block
		while (cp + dp->rec_len < buff + BLKSIZE)
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		//entry's needed length always multiple of 4
		need_len = (4*((8+dp->name_len+3)/4));

		remain = (dp->rec_len) - need_len;

		if (remain >= need_len)
		{
			//trim last entry's rec_len to ideal/need_len
			dp->rec_len = need_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			dp->inode = myino;
			//enter new entry as last entry with rec_len = remain

			dp->rec_len = remain;
			dp->name_len = strlen(myname);
			strcpy(dp->name, myname);
			put_block(pmip->dev, pmip->INODE.i_block[i], buff);
			flag = 1;
		}
	}
	if (flag == 0)
	{ //if remain >= need_len is not true or i_block[i] == 0 allocate new block
		//set entry as first data block and increase DIR size by BLKSIZE

		pmip->INODE.i_block[i] = balloc(dev);
		pmip->INODE.i_size += BLKSIZE;
		pmip->INODE.i_blocks += 2;

		//get i_block and change rec_len to BLKSIZE, put i_block back to disk
		get_block(pmip->dev, pmip->INODE.i_block[i], buff);
		dp = (DIR *)buff;
		cp = buff;
		dp->rec_len = BLKSIZE;
		dp->name_len = strlen(myname);
		dp->inode = myino;
		strcpy(dp->name, myname);
		put_block(pmip->dev, pmip->INODE.i_block[i], buff);
	}
}

/*Returns the inode number of a pathname*/
int getino(int *dev, char *pathname)
{
	int i, ino, blk, disp;
	char buff[BLKSIZE];
	INODE *ip;
	MINODE *mip;

	if (strcmp(pathname, "/") == 0)
		//2 is the roots INODE number
		return 2;

	if (pathname[0] == '/')
	{ //absolute pathname
		mip = iget(*dev, 2);
	}
	else
	{ //get running minode
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

		//Search for that tokenized pathname, return the ino if found
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

  for (i = 0; i < nblocks; i++)
	{
    if (tst_bit(buf, i) == 0)
		{
      set_bit(buf,i);
      put_block(dev, bmap, buf);
      decFreeBlocks(dev);
      printf("Block allocated:%d\n", i + 1);
      return i + 1;
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
  for (i = 0; i < ninodes; i++)
	{
    if (tst_bit(buf, i) == 0)
		{
       set_bit(buf,i);
       decFreeInodes(dev);
       put_block(dev, imap, buf);
       return i + 1;
    }
  }
  printf("No more free inodes\n");
  return 0;
}

int mymkdir(MINODE *pmip, char *name)
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
		tip->i_block[i] = 0;	//Iterates through all blocks, initializing

	mip->dirty = 1;

	iput(mip);
	get_block(dev, tip->i_block[0], buff);

	dp = (DIR *)buff;
	cp = dp;

	dp->inode = tempIno;
	printf("tempino: %d\n", tempIno);
	dp->rec_len = 12;
	dp->name_len = 1;
	strcpy(dp->name, ".");	//Points to THIS dir

	cp += dp->rec_len;
	dp = (DIR *)cp;

	dp->inode = pmip->ino;
	printf("pmip->ino: %d\n",pmip->ino);
	dp->rec_len = BLKSIZE-12;
	dp->name_len = 2;
	strcpy(dp->name, "..");	//Points to parent

	/*Update touch time*/
	pmip->INODE.i_atime = time(0L);
	pmip->INODE.i_links_count += 1;
	pmip->dirty = 1;
	pmip->INODE.i_mode = 0040000;

	put_block(dev, tempBno, buff);
	enter_name(pmip, tempIno, name);
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
{	//all direct nodes, deallocat all block
	for (int i = 0; i < 12; i++)
	{
		if (mip->INODE.i_block[i] == 0)
			break;
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	mip->dirty = 1;
	mip->INODE.i_size = 0;
}
