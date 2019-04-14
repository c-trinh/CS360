#ifndef _UTILC_
#define _UTILC_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "iget_iput_getino.c"

int pwdPrint(MINODE *hold)
{
	MINODE *temp;
	char *pops = "..";


	if (hold->ino == 2)
	{
		return;
	}
	
	//Grab the ino.
	int tempIno = getino(&hold->dev, pops);
	temp = iget(hold->dev, tempIno);
	//temporarily make this the cwd
	running->cwd = temp;
	
	pwdPrint(temp);
	childPrint(temp, hold->ino);
	iput(temp);
}

int childPrint (MINODE *temp, int ino)
{
	
	char *cp;
	char buff[BLKSIZE];

	ip = &(temp->INODE);

	for (int i = 0; i < 12; i++)
	{
		if (ip->i_block[i] == 0)
			return;

		get_block(dev, ip->i_block[i], buff);
		//step through.
		dp = (DIR *)buff;
		cp = dp;

		while (cp < &buff[1024])
		{
			 
			if (dp->inode == ino)
			{ 
				printf("/%s", dp->name);
			}
			 
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	return;
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

int my_creat(MINODE *pip, char *name)
{
	int tempIno, tempBno;
	MINODE *mip;
	char buff[BLKSIZE];
	char *cp;

	 
	tempIno = ialloc(dev);
	tempBno = balloc(dev);

	//Get memory.
	mip = iget(dev, tempIno);

	//Temporary inode fill out the new memory slot.
	INODE *tip = &mip->INODE;

	tip->i_mode = 0x81A4;
	tip->i_uid = running->uid;
	tip->i_gid = 0;
	tip->i_size = 0;
	tip->i_links_count = 1;
	tip->i_atime = tip->i_ctime = tip->i_mtime = time(0L);  
	tip->i_blocks = 2;
	tip->i_block[0] = tempBno;

	for (int i = 1; i < 15; i++)
	{
		tip->i_block[i] = 0;
	}

	mip->dirty = 1;
	iput(mip);
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

int rm_child(MINODE *pip, char *name)
{
	char buff[BLKSIZE], namebuff[BLKSIZE], removebuff[BLKSIZE];
	char *cp, *tempcp;
	int ideal_len, cur_len, prev_len, mem_to_move, run_total, temphold;

	for (int i = 0; i < BLKSIZE; i++)
	{
		removebuff[i] = 0;
	}
	for (int i = 0; i < 12; i++)
	{
		get_block(dev, pip->INODE.i_block[i], buff);
		dp = (DIR *)buff;
		cp = buff;
		tempcp = buff;

		//how far we've gone.
		run_total = 0;

		//Search for name
		while (cp < &buff[BLKSIZE])
		{
			//Get the name into a buff
			strcpy(namebuff, dp->name);
			//Tricky null terminator
			namebuff[dp->name_len] = 0;
			//Tally the running total.
			run_total += dp->rec_len;
			cur_len = dp->rec_len;
			ideal_len = (4*((8+dp->name_len + 3)/4));

			if (strcmp(namebuff, name) == 0)
			{
				if (dp->rec_len == BLKSIZE)
				{
					printf("HERE.\n");
					bdealloc(dev, pip->INODE.i_block[i]);
					//Zero it out.
					pip->INODE.i_block[i] = 0;
					//Decrement parent
					pip->INODE.i_size -= BLKSIZE;
					//Move everything forward.
					int k = i;
					while (k < 12 && pip->INODE.i_block[k+1])
					{
						pip->INODE.i_block[k+1] = pip->INODE.i_block[k];
					}
					put_block(dev, pip->INODE.i_block[i], buff);
					return;
				}
				else if (dp->rec_len > ideal_len)
				{
					//Move the pointer back
					cp -= prev_len;
					dp = (DIR *)cp;
					dp->rec_len += cur_len;
				}
				else
				{
					 
					int len = BLKSIZE - ((cp+cur_len) - buff);
					printf("Length %d\n", len);
					memmove(cp, cp+cur_len, len);
					printf("REMOVE\n");
					 
					while (cp + dp->rec_len < &buff[1024] - cur_len)
					{
						printf("cp: %d buff: %d\n", cp, &buff[1024]);
						printf("Name: %s\n", dp->name);
						cp += dp->rec_len;
						dp = (DIR *)cp;
					}
					//add the length that was deleted.
					dp->rec_len += cur_len;
				}
				put_block(dev, pip->INODE.i_block[i], buff);
				return;
			}
			prev_len = dp->rec_len;
			tempcp += dp->rec_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	
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

 
#endif
