/*assist.c*/

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
	{	//Iterates through direct blocks
		get_block(mip->dev, mip->INODE.i_block[i], dbuf);
		dp = (DIR *)dbuf;
		cp = dbuf;
		if (mip->INODE.i_block[i] == 0)
			return 0;
		while (cp < &dbuf[1024])
		{
			strncpy(sbuf, dp->name, dp->name_len);
			sbuf[dp->name_len] = 0;

			if (strcmp(sbuf, name) == 0)
			{
				printf("Directory located @ INODE: %d\n", dp->inode);
				return dp->inode;
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	sys_msg('!', "Directory not found in CWD.");
}


/*Returns a pointer to the in-memory INODE of (dev,ino)
and increases reference count based on how many shared objects*/
MINODE *iget(int dev, int ino)
{
	int i, blk, disp;
	char buf[BLKSIZE];
	MINODE *mip;
	INODE *ip;
	for (i = 0; i < NMINODE; i++) {
		mip = &minode[i];
		if (mip->dev == dev && mip->ino == ino)
		{
			mip->refCount++;
			return mip;
		}
	}
	for (i = 0; i < NMINODE; i++) {
		mip = &minode[i];
		if (mip->refCount == 0) {
			mip->refCount = 1;
			mip->dev = dev; mip->ino = ino;  // assing to (dev, ino)
			mip->dirty = mip->mounted = mip->mptr = 0;
			// get INODE of ino into buf[ ]
			blk = (ino - 1) / 8 + iblock;  // iblock = Inodes start block #
			disp = (ino - 1) % 8;
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

int rm_child(MINODE *pip, char *baseName)
{
	char buff[BLKSIZE], buffName[BLKSIZE];
	char *cp, *old_cp;
	int max_len, cur_len, prev_len;

	for (int i = 0; i < 12; i++)
	{	//Recursive through direct inodes
		get_block(dev, pip->INODE.i_block[i], buff);
		dp = (DIR *)buff;
		old_cp = buff;
		cp = buff;

		while (cp < &buff[BLKSIZE])	//Search for name
		{
			strcpy(buffName, dp->name);	//Get the name into a buff
			buffName[dp->name_len] = 0;	//Resets name
			cur_len = dp->rec_len;
			max_len = (4 * ((8 + dp->name_len + 3) / 4));

			if (strcmp(buffName, baseName) == 0)
			{
				if (max_len < dp->rec_len)
				{	//dir pointer exceeds max length
					cp -= prev_len;
					dp = (DIR *)cp;
					dp->rec_len += cur_len;	//digress pointer
				}
				else if (dp->rec_len == BLKSIZE)
				{
					bdealloc(dev, pip->INODE.i_block[i]);
					pip->INODE.i_block[i] = 0;		//Set iblock to empty
					pip->INODE.i_size -= BLKSIZE;	//Decremenr parent size
					int j = i;
					while (j < 12 && pip->INODE.i_block[j + 1])
					{
						pip->INODE.i_block[j + 1] = pip->INODE.i_block[j];
					}
					put_block(dev, pip->INODE.i_block[i], buff);

					return;	//Ends function
				}
				else
				{
					int len = BLKSIZE - ((cp + cur_len) - buff);
					printf("Length %d\n", len);
					memmove(cp, cp + cur_len, len);
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
				return;	//Ends function
			}
			prev_len = dp->rec_len;
			old_cp += dp->rec_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}

void readlink()
{
	int temp_ino;
	char *cp;
	MINODE *mip;

	temp_ino = getino(&dev, pathname);

	if (temp_ino == 0)
	{
		printf("This path does not exist.\n");
		return;
	}
	mip = iget(dev, temp_ino);

	if (mip->INODE.i_mode != 0xA000)
	{
		printf("This is not a symbolic link\n");
		return;
	}

	printf("%s\n", mip->INODE.i_block);

	printf("\n");

}

/*Releases and unlocks a minode pointed by mip. (Decreases reference count)
If process is last to use minode, INODE written back to disk if dirty*/
int iput(MINODE *mip)
{
	int offset, block;
	char buff[1024];

	mip->refCount--;

	if (mip->refCount > 0) {
		return;
	}
	if (!mip->dirty) {
		return;
	}

	//printf("iput: dev = %d ino = %d\n", mip->dev, mip->ino);

	block = (mip->ino - 1) / 8 + iblock;
	offset = (mip->ino - 1) % 8;

	get_block(mip->dev, block, buff);

	ip = (INODE *)buff + offset;
	*ip = mip->INODE;

	put_block(mip->dev, block, buff);
}


int tst_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8; j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

int set_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8; j = bit % 8;
	buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8; j = bit % 8;
	buf[i] &= ~(1 << j);
}
