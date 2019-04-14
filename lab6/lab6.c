#define BLKSIZE 1024

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ext2type.h"

typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

struct ext2_group_desc *gp;
struct ext2_super_block *sp;
struct ext2_inode *ip;
DIR *dirPtr;
int fd;

void searchDoubleInd(int blockNum2)
{
    int offset = 0;
    char buf[BLKSIZE];
    get_block(fd, blockNum2, buf);
    int* indptr = (int*)buf;
    while(offset < 256)
    {
	if(*(indptr + offset) == 0) {break;}
	printf("Double Indirect block %d: %d\n", offset, *(indptr + offset));
	offset++;
    }

}

void searchInd2(int blockNum)
{
    int offset = 0;
    char buf[BLKSIZE];
    get_block(fd, blockNum, buf);
    int* indptr = (int*)buf;

    while(offset < 256)
    {
        if(*(indptr + offset) == 0) {break;}
        printf("2 Indirect block %d: \n", *(indptr + offset));
	searchDoubleInd(*(indptr + offset));
        offset += 1;
    }
}

void searchInd(int blockNum)
{
    int offset = 0;
    char buf[BLKSIZE];
    get_block(fd, blockNum, buf);
    int* indptr = (int*)buf;

    while(offset < 256)
    {
	if(*(indptr + offset) == 0) {break;}
	printf("Indirect block %d: %d\n", offset, *(indptr + offset));
	offset += 1;
    }
}

struct ext2_inode* search(char* tokName, struct ext2_inode* in)
{
	struct ext2_inode* tempInode = in;
	//struct DIR* dirPtr;
	char buf[BLKSIZE];
	int i = 0;
	char *cp;
  int isFound = 0;
	get_block(fd, in->i_block[0], buf);
	dirPtr = (struct DIR *) buf;
	cp = buf;
	if (dirPtr->inode == 0) return NULL;
	while(cp < &buf[1024])
	{
		printf("Inode number: %u\n", dirPtr->inode);
		printf("rec_len: %u\n", dirPtr->rec_len);
		printf("name_len = %d\n", dirPtr->name_len);
		printf("name = %s\n\n", dirPtr->name);

		if (strcmp(dirPtr->name, tokName) == 0)
		{
			printf("Inode found!\n");
			isFound = 1;
			break;
		}

		cp += dirPtr->rec_len;
		dirPtr = (struct DIR*) cp;
	}

	if(isFound)
	{

		printf("Inode found!\n");
		printf("Inode #: %d\n", dirPtr->inode);
		printf("Record Length: %d\n", dirPtr->rec_len);
		printf("Name len: %d\n", dirPtr->name_len);
		printf("File Type: %d\n", dirPtr->file_type);
		printf("Name: %s\n", dirPtr->name);

		// get inode start block
		int ino = dirPtr->inode;

		get_block(fd, 1, buf); //read super block
		sp = (struct ext2_super_block *)buf;

		get_block(fd, 1 + sp->s_first_data_block, buf);//read group descriptor
		gp = (struct ext2_group_desc *)buf;

		get_block(fd, ((ino - 1) / 8) + gp->bg_inode_table, buf); //THEN find the inode

		tempInode = (struct ext2_inode *)buf + ((ino-1) % 8);         // offset

		printf("---------------Direct Blocks--------------\n");
		while(i < 12)
		{
	if(tempInode->i_block[i] != 0)
		printf("Block[%d]: %d\n", i, tempInode->i_block[i]);
				i++;
		}

		if(tempInode->i_block[12] == 0)
		{
			 printf("No indirect blocks\n");
		}

		else
		{
	searchInd(tempInode->i_block[12]);
		}

		if(tempInode->i_block[13] == 0)
		{
	printf("No double indirects\n");
		}

		else
		{
	searchInd2(tempInode->i_block[13]);
		}

	}
	return tempInode;

	//return;

}

void inode (char* pathname, struct ext2_inode* in)
{
	char buf[BLKSIZE];
	char* name[64];
	int i = 0, j = 0;	//i = How many directories, j = Increment as i decrement

	printf("%s\n", pathname);
	name[0] = strtok(pathname, "/");	//Sets strtok, places seperated paths in individual arrays


  while(name[i] != NULL)	//Tokenizes pathname by '/'
	{
		i++;	//Counts amount of dir
		name[i] = strtok(NULL, " /");
	}

	printf("mode=%4x\n ", in->i_mode);
  printf("uid=%d\n  gid=%d\n", in->i_uid, in->i_gid);
  printf("size=%d\n", in->i_size);
  printf("time=%s", ctime(&in->i_ctime));
  printf("link=%d\n", in->i_links_count);
  printf("i_block[0]=%d\n", in->i_block[0]);

	while (i - 1>= 0)
  {
		in = search(name[j], in);
	  i--;	//Decrements counted directories
		j++;
  }
}

void get_block(int fd, int blk, char buf[])	//blk# = type
{
  lseek(fd,(long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

int main(int argc, char *argv[])
{
	struct ext2_super_block *sb;	//From ext2type.h
	struct ext2_group_desc *gd;
	struct ext2_inode *in;

	char* disk = argv[1];
	char* path = argv[2]; //Parses arguments
	char buf[BLKSIZE];

	fd = open(disk, O_RDONLY);	//Opening disk for reading

	get_block(fd, 1, buf);	//Gets super block
	sb = (struct ext2_super_block *)buf;	//Casts buffer to superblock type

	printf("magic number: %x\n", sb->s_magic);	//Prints magic number hex 0xEF53 - Checks that its a ext2type

	get_block(fd, sb->s_first_data_block + 1, buf);	//Loading group descrintor into buff
	gd = (struct ext2_group_desc*)buf;						//Casts buf to ext2_group_desc type

	printf("inode begin block: %d\n", gd->bg_inode_table);	//Prints begin_block of inode

	get_block(fd, gd->bg_inode_table, buf);	//Loading beginning of inode table into buff
	printf("datablock: %d\ntable: %d\n", sb->s_first_data_block+1, gd->bg_inode_table);
	in = (struct ext2_inode*)buf + 1;
	printf("before inode function call\n");
	inode(path, in);	//Calls inode, passes in path from argv[2]

	return 0;
}
