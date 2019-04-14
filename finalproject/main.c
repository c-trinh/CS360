/*main.c*/

/*	Coders:			Cong Trinh, Jeffrey Walsh
*	Professor:		K.C. Wang
*	Course:			Cpts360
*	Final Project:	EXT2 file system
*	Description:	The project is a linux-compatible EXT2 file system.
*						that uses a disk (Which can be formatted by default
*						w/ the provided mk.sh file running "sudo mk.sh").
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include "util.c"

int init()
{	//Sets values to default / populates
	int i, j;
	MINODE *mip;
	PROC   *p;

	sys_msg('~', "Executing \'init\'...");
	for (i = 0; i < NMINODE; i++) {
		mip = &minode[i];
		mip->dev = mip->ino = 0;
		mip->refCount = 0;
		mip->mounted = 0;
		mip->mptr = 0;
	}
	for (i = 0; i < NPROC; i++) {
		p = &proc[i];
		p->pid = i;
		p->cwd = 0;
		p->uid = 0;

		for (j = 0; j < NFD; j++)
		{
			p->fd[j] = 0;
		}
	}
	sys_msg('+', "Successfully executed \'init\'.");
}

int lookup(char *cmd)
{
	for (int i = 0; i < 14; i++)
	{
		if (!strcmp(cmdMenu[i], cmd))
			return i;
	}
	return -1;	//Command not found

}

int mount_root()
{
	sys_msg('~', "Function \'mount_root()\' executing...");
	MNTABLE mntable, *mntPtr;
	root = iget(dev, 2);

	root->mptr = &mntable;
	root->mounted = 1;

	mntPtr = &mntable;
	mntPtr->mntDirPtr = root;
	mntPtr->dev = dev;
	mntPtr->ninodes = ninodes;
	mntPtr->nblocks = nblocks;
	mntPtr->imap = imap;
	mntPtr->bmap = bmap;
	mntPtr->iblk = iblk;
	strcpy(mntPtr->devName, "mydisk");
	strcpy(mntPtr->mntName, "/");

	sys_msg('+', "Successfully mounted root to the disk.");
}

/******************************************************************
** _                         LEVEL 1                          _ **
******************************************************************/

/*<----beginning of ls---->*/
void ls_file(MINODE *mip, char *fName)
{
	char tempDate[32], *fDate, *cp, fTime[32];
	int i;
	u16 mode, mask;
	//printf("TYPE:\tDATE\tTIME\tNAME");
	mode = mip->INODE.i_mode;
	if (S_ISDIR(mode))
		printf("DIR");
	else if (S_ISLNK(mode))
		printf("LINK");
	else
		printf("FILE");
	putchar('\t');

	fDate = tempDate;
	fDate = (char *)ctime(&mip->INODE.i_ctime);
	fDate = fDate + 4;
	strncpy(fTime, fDate, 12);
	fTime[12] = 0;

	printf("%s", fTime);				//Prints time stamp
	printf("%8ld", mip->INODE.i_size);	//Prints block size
	printf("\t%s", fName);				//Prints directory name

	if (S_ISLNK(mode))
		printf(" -> %s", (char *)mip->INODE.i_block);
	printf("\n");

}

void ls_dir(MINODE *mip)
{
	int i;
	char buf[BLKSIZE], dirName[256];
	DIR *dp;	//direct pointer
	char *cp;	//current pointer
	MINODE *ip;	//inode pointer

	for (i = 0; i < 12; i++) { //iterates/searches direct blocks only
		if (mip->INODE.i_block[i] != 0)		//if iblocks of inode is NOT empty (Safecheck)
		{
			get_block(mip->dev, mip->INODE.i_block[i], buf); // if i_block is not empty get_block called, read into sbuf
			dp = (DIR *)buf;	//cast to DIR * type
			cp = buf;					//point current pointer to 1st index of buf

			while (cp < buf + BLKSIZE) {			//while index of buf does not go out of range
				strncpy(dirName, dp->name, dp->name_len);	//dirName = name trimmed w/ length
				dirName[dp->name_len] = 0;					//set last character to null pointer for string
				ip = iget(dev, dp->inode);					//get directory inode-pointer from dp->inode
				ls_file(ip, dirName);						//call to ls_file to complete ls
				iput(ip);									//put directory inode pointer

				cp += dp->rec_len;	//increment cp by record length
				dp = (DIR *)cp;		//set directory pointer to new directory pointer to ls on next loop
			}
		}
		else	//Breaks loop
			return;

	}
}

int my_ls()
{	//Figures whether to call ls_dir or ls_file
	MINODE *mip;	//memory inode pointer
	u16 mode;
	int dev, ino;

	if (pathname[0] == '/')	//Relative pathname
	{	//Absolute pathname
		dev = root->dev;	//set device to point at root's device
		printf("%s\n", pathname);
		ino = getino(&dev, pathname);	//gets ino from pathname
		if (ino != 0) {				//Pathname exists in CWD
			mip = iget(dev, ino);			//inode in memory is set to the value in iget with device and ino
			mode = mip->INODE.i_mode;	//set mode to inode in memory pointer's mode
			if (!S_ISDIR(mode))		//is a file
				ls_file(mip, (char*)basename(pathname));	//ls file
			else					//is a dir
				ls_dir(mip);								//ls dir
			iput(mip); // puts blocks from minode pointer and sets the inode to that value
		}
		else	//Pathname not found in CWD
		{
			printf("\n[!]-\"ERROR: Pathname \'%s\' not found in CWD.\"\n", pathname);
			return 2;
		}

	}
	else	//Relative pathname
		ls_dir(running->cwd);
	return 1;
}
/*<----end of ls---->*/

/*<----beginning of cd---->*/
int chdir()
{
	int base;		//Destination of inode; Directory being changed to
	MINODE *mip;	//Memory inode pointer

	if (strcmp(pathname, "") == 0) { 	//If Pathname is empty, reload cwd (Safecheck)	//Brings back to root
		iput(running->cwd);							//releases/unlocks cwd + decrements ref. to overwrite w/ new ref
		running->cwd = iget(dev, 2);		//Retrives memory pointer of cwd
	}
	else
	{
		if (pathname[0] == '/') //Is it absolute
			dev = root->dev;         //Sets dev to root's device
		else                    //not absolute
			dev = running->cwd->dev; //Set dev to cwd's device

		base = getino(&dev, pathname); //Retrieves ino to compare

		if (base != 0)	//Pathname found
		{					//Pathname found
			mip = iget(dev, base);	//Retrives memory pointer of pathname inputted by user
			if (S_ISDIR(mip->INODE.i_mode))
			{  //inode is a directory
				iput(running->cwd);		//Clears reference for cwd
				running->cwd = mip;		//Sets new cwd
			}
			else
			{  //inode is a file
				iput(mip);	//Removes refence from mip; No inode found
				sys_msg('!', "ERROR: Destination is a File type.");
			}
		}
	}
}
/*<----end of cd---->*/

/*<----beginning of pwd---->*/
MINODE *find_parent(MINODE *mip)
{
	char *parent = "..", *cp;
	char buff[BLKSIZE];
	get_block(mip->dev, mip->ino, buff);
	dp = (DIR *)buff;
	cp = dp;
	cp += dp->rec_len;
	dp = (DIR *)cp;
	printf("%d\n", dp->inode);
	getchar();
}

void print_dir(MINODE *temp, int ino)
{
	char *cp;
	char buff[BLKSIZE];
	ip = &(temp->INODE);
	for (int i = 0; i < 12; i++)
	{	//Iterates through direct iblocks
		if (ip->i_block[i] != 0)
		{	//iblock is not empty
			get_block(dev, ip->i_block[i], buff);
			dp = (DIR *)buff;
			cp = dp;								//Current Pointer = dir pointer
			while (cp < &buff[1024])
			{
				if (dp->inode == ino) 	//If inode finds basename's mip ino
					printf("/%s", dp->name);	//Prints the dirname (directory path)
				cp += dp->rec_len;
				dp = (DIR *)cp;							//Updated dir pointer
			}
		}
		else	//If iblock is empty
			return;	//finished printing
	}
}

void rpwd(MINODE *mip)
{
	MINODE *pmip;

	if (mip->ino == 2)
		return;	//Return on empty inode

	int parent = getino(&mip->dev, "..");
	pmip = iget(mip->dev, parent);	//Gets directory inode
	running->cwd = pmip;			//Changes cwd
	rpwd(pmip);								//Recursive through parent mip
	print_dir(pmip, mip->ino);//Prints directory
	iput(pmip);								//Releases pmip
}

void pwd()
{
	MINODE *mip;
	putchar('\n');
	if (running->cwd->ino == 2)
		printf("/");	//if cwd is a root
	mip = running->cwd;	//Gets mip of cwd
	rpwd(mip);			//Recursively prints working directory
	running->cwd = mip;
	iput(mip);
	putchar('\n');
}
/*<----end of pwd---->*/

/*<----beginning of mkdir---->*/
void exec_mkdir()
{
	char parent[256], child[256];	//Temp. stores dirnam and basename
	char *dirName, *baseName;
	int pino;
	MINODE *mip, *pmip;

	if (pathname[0] == '/')	//Is absolute
		dev = root->dev;			//Sets dev to root's device
	else					//Not absoulute
		dev = running->cwd->dev;	//Sets dev to cwd's device

	/*Splits dirName and baseName*/
	strcpy(parent, pathname);
	dirName = dirname(parent);

	strcpy(child, pathname);
	baseName = basename(child);

	/*gets inode of parent (dirName)*/
	pino = getino(&dev, dirName);
	pmip = iget(dev, pino);

	if (!S_ISDIR(pmip->INODE.i_mode))
	{	//if is a file
		sys_msg('!', "ERROR: FILE type is invalid path.");
		return;
	}
	if (getino(&dev, pathname))
	{	//if exists
		sys_msg('!', "ERROR: DIR already exists in CWD.");
		return;
	}

	mymkdir(pmip, baseName);	//mkdir helper
	iput(pmip);					//Releases mip of parent
}
/*<----end of mkdir---->*/

/*<----beginning of rmdir---->*/
void my_rmdir()
{
	int ino, pino;		//ino, parent ino
	MINODE *mip, *pmip;
	char buff[BLKSIZE];
	char *cp;

	//Gets in-memory INODE of pathname
	ino = getino(&dev, pathname);
	mip = iget(dev, ino);

	if (ino == 0)
		sys_msg('!', "ERROR: Pathname not recognized.");
	else if (S_ISDIR(mip->INODE.i_mode))
	{
		get_block(dev, mip->INODE.i_block[0], buff);

		dp = (DIR *)buff;		//Directory pointer
		cp = buff;					//Current pointer
		cp += dp->rec_len;	//cp = buf + length of dp
		dp = (DIR *)cp;			//dp = cp to dir

		pino = dp->inode;
		if (dp->rec_len == 1012)	//Directory is Empty	//Check if refcount = 1
		{
			/*Deallocate datablocks + inode*/
			truncate(mip);
			idealloc(dev, mip->ino);

			iput(mip);							//Decrement reference
			pmip = iget(dev, pino);	//Get pino from .. entry in iblock[0]

			rm_child(pmip, basename(pathname));	//Removes name from parent directory

			pmip->INODE.i_links_count--;
			pmip->INODE.i_atime = time(0L);
			pmip->dirty = 1;
			iput(pmip);
		}
		else	//Directory is not empty
			sys_msg('!', "INVALID: Directory is not empty.");
	}
}
/*<----end of rmdir---->*/

/*<----beginning of creat---->*/
void creat_file(MINODE *pip, char *name)
{
	int ino, bno;
	MINODE *mip;
	char buff[BLKSIZE];
	char *cp;
	/*Allocate memory for inode*/
	ino = ialloc(dev);
	bno = balloc(dev);

	mip = iget(dev, ino);	//Stores memory

	INODE *omip = &mip->INODE;	//Temporary inode fill out the new memory slot.

	omip->i_mode = 0x81A4;
	omip->i_uid = running->uid;
	omip->i_gid = 0;
	omip->i_size = 0;
	omip->i_links_count = 1;
	omip->i_atime = omip->i_ctime = omip->i_mtime = time(0L);

	omip->i_blocks = 2;
	omip->i_block[0] = bno;

	for (int i = 1; i < 15; i++)
		omip->i_block[i] = 0;	//Clears iblocks

	mip->dirty = 1;	//inode is empty (since is file)]

	pip->INODE.i_atime = time(0L);
	pip->INODE.i_links_count = 1;
	pip->dirty = 1;

	iput(mip);		//Returns to disk
	enter_name(pip, ino, name);
}

void my_creat()
{
	MINODE *pip;
	char parent[256], child[256];
	char *dirName, *baseName;
	if (pathname[0] == '/')
	{	//Absolute path
		dev = root->dev;
	}
	else
	{	//Relative path
		dev = running->cwd->dev;
	}

	strcpy(parent, pathname);
	strcpy(child, pathname);
	dirName = dirname(parent);
	baseName = basename(child);

	//Grab MINODE of parent.
	int pino = getino(&dev, dirName);
	pip = iget(dev, pino);

	if (search(pip, pathname))
	{
		sys_msg('!', "ERROR: Name already exists on disk.");
		return;
	}

	creat_file(pip, baseName);

	iput(pip);
}
/*<----end of creat---->*/

/*<----beginning of touch---->*/
int touch()
{
	int ino, i = 0;
	char *cp;
	int dev = running->cwd->dev;
	MINODE *mip = running->cwd, *tip;
	char buff[1024], buff2[1024];

	if (strcmp(pathname, "") != 0)
	{
		if (pathname[0] == '/') 	//Absolute path
			dev = root->dev;

		ino = getino(&dev, pathname);
		if (ino == 0)
			return;
		mip = iget(dev, ino);	//get minode pointer
	}
	else
		ino = running->cwd->ino; //else if not get cwd inonumber

	while (mip->INODE.i_block[i]) //go through datablocks
	{
		get_block(dev, mip->INODE.i_block[i], buff);
		cp = buff;
		dp = (DIR *)buff;
		while (cp < &buff[1024])
		{
			tip = iget(dev, dp->inode);
			strncpy(buff2, dp->name, dp->name_len);
			buff2[dp->name_len] = 0;
			putchar('\n');

			mip->INODE.i_ctime = time(0L);
			mip->INODE.i_mtime = time(0L);
			mip->INODE.i_atime = time(0L); //touch times
			mip->dirty = 1;
			iput(mip);

			cp += dp->rec_len;
			dp = (DIR *)cp;
			return;
		}
		i++;
	}
}
/*<----end of touch---->*/

/*<----beginning of link---->*/
void my_link()
{
	int oino, pino;		//old ino, parent ino
	MINODE *mip, *omip;	//memory inode pointer, temp inode pointer
	char parent[64], child[64];
	char *dirName, *baseName;


	oino = getino(&dev, pathname);
	mip = iget(dev, oino);	//Gets minode pointer from ino

	if (S_ISDIR(mip->INODE.i_mode))
	{  //Can not link a DIR type
		sys_msg('!', "ERROR: Link can not be applied to DIR.");
	}
	else if (strcmp(parameter, "") == 0)
	{
		sys_msg('!', "ERROR: Link needs a Parameter.");
	}
	else if (oino != 0)
	{	//Pathname is found
		strcpy(parent, parameter);
		dirName = dirname(parent);	//Stores directory pathname

		strcpy(child, parameter);
		baseName = basename(child);	//Stores destination name

		pino = getino(&dev, dirName);	//Stores parent inode
		if (pino != 0) //parent (directory pathname) does exist
		{
			omip = iget(dev, pino);				//Retrieves inode pointer of parent
			enter_name(omip, oino, baseName);	//Enters directory basename

			omip->INODE.i_links_count++;
			omip->dirty = 1;
			/*releases from memory and put back into device*/
			iput(omip);
			iput(mip);
		}
		else
			sys_msg('!', "ERROR: Parent does not exist.");
	}	// Else: pathname not found.
}
/*<----end of link---->*/

/*<----beginning of unlink---->*/
void unlink()
{
	int path_ino, par_ino;
	char *baseName, *dirName, child[128], parent[128];
	MINODE *mip, *pmip;

	path_ino = getino(&dev, pathname);
	mip = iget(dev, path_ino);
	if (path_ino == 0)
		sys_msg('!', "ERROR: Pathname not recognized");
	else if (!S_ISDIR(mip->INODE.i_mode))
	{	//Makes sure inode is not a directory (is a file)
		mip->INODE.i_links_count--;	//Decrement inode's link count

		strcpy(child, pathname);	//Stores directory pathname
		baseName = basename(child);

		strcpy(parent, pathname);	//Stores pathname
		dirName = dirname(parent);

		par_ino = getino(&dev, dirName);
		pmip = iget(dev, par_ino);

		rm_child(pmip, baseName);	//Remove child/basename

		if (mip->INODE.i_links_count > 0)
		{	//Is associated with links/connections
			mip->dirty = 1;
		}
		else if (mip->INODE.i_links_count == 0)
		{	//Has no links/connections
			truncate(mip);
			idealloc(dev, mip->ino);
		}
		iput(pmip);
		iput(mip);
	}
	else	//inode is a directory
		sys_msg('!', "ERROR: Unlink can not be applied to DIR.");
}
/*<----end of unlink---->*/

/*<----beginning of symlink---->*/
void symlink()
{
	//assuming oldname (pathname) only has 60 chars...
	char oldname[84], temppath2[128], temppath1[128];
	char *basen, *dirn, *write_in;
	int oino, par_ino;
	MINODE *pmip, *mip;
	//This is the pathname, i.e /a/b/c
	strncpy(oldname, pathname, 60);

	//Let's get base and dir
	strcpy(temppath1, parameter);
	strcpy(temppath2, parameter);
	dirn = dirname(temppath1);
	basen = basename(temppath2);

	oino = getino(&dev, parameter);
	if (oino != 0)
	{
		printf("This file already exists\n");
		return;
	}
	//This means they are the same and not absolute.
	printf("Dirn: %s basen: %s param: %s\n", dirn, basen, parameter);
	if (strcmp(basen, parameter) == 0)
	{	//If the same
		par_ino = running->cwd->ino;
		pmip = iget(dev, par_ino);
		creat_file(pmip, basen);
	}
	else
	{
		par_ino = getino(&dev, dirn);

		pmip = iget(dev, par_ino);
		printf("\n\npar_ino:%d dirn: %s\n", par_ino, dirn);
		creat_file(pmip, basen);
	}

	//Write oldname into that new file.
	oino = getino(&dev, parameter);
	mip = iget(dev, oino);
	mip->INODE.i_mode = 0xA000;
	//write_in = (char*)mip->INODE.i_block;
	strcpy(mip->INODE.i_block, pathname);
	iput(mip);
}
/*<----end of symlink---->*/

/*<----beginning of stat---->*/
void my_stat() {
	int tempIno, i = 0;
	char *cp;
	int dev = running->cwd->dev;
	MINODE *mip = running->cwd, *omip;
	char buff[1024], buff2[1024];
	if (pathname[0] != 0)
	{
		if (pathname[0] == '/')
		{
			dev = root->dev;
		}

		tempIno = getino(&dev, pathname);
		if (tempIno == 0)
			return;
		mip = iget(dev, tempIno);
	}
	else
	{
		tempIno = running->cwd->ino;

		mip = running->cwd;
	}
	while (mip->INODE.i_block[i])
	{
		get_block(dev, mip->INODE.i_block[i], buff);
		cp = buff;
		dp = (DIR *)buff;
		while (cp < &buff[1024])
		{
			omip = iget(dev, dp->inode);
			strncpy(buff2, dp->name, dp->name_len);
			buff2[dp->name_len] = 0;
			printf("SB inode count: %d\n", sp->s_inodes_count);
			printf("pathname: %s\n", pathname);
			printf("mode: 0x%x\n", mip->INODE.i_mode);
			printf("record length: %d\n", dp->rec_len);
			printf("inode number: %d\n", dp->inode);
			printf("size: %d\n", mip->INODE.i_size);
			printf("link count: %d\n", mip->INODE.i_links_count);
			printf("UID: %d\n", mip->INODE.i_uid);
			printf("GID: %d\n", mip->INODE.i_gid);
			printf("ctime: %s", asctime(gmtime(&mip->INODE.i_ctime)));
			printf("atime: %s", asctime(gmtime(&mip->INODE.i_atime)));
			printf("mtime: %s", asctime(gmtime(&mip->INODE.i_mtime)));
			if (S_ISDIR(mip->INODE.i_mode)) { 	//imode returns non-zero if dir
				printf("inode is a DIR type.\n");
			}
			else {
				printf("inode is a FILE type.\n\n");
				iput(omip);
				return;
			}
			iput(omip);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		i++;
	}
	printf("\n");
	if (mip != running->cwd)
	{
		iput(mip);
	}
}
/*<----end of stat---->*/


void my_chmod()
{
	int temp_ino;
	MINODE *mip;
	char *mode;
	int permission;
	sscanf(pathname, "%d", &mode);
	if (parameter[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}
	temp_ino = getino(&dev, parameter);	//3rd is basename
	if (temp_ino == 0)
		return;
	mip = iget(dev, temp_ino);

	mip->INODE.i_mode = mode;
	mip->dirty = 1;

	iput(mip);
}


/*<----beginning of quit---->*/
void my_quit(void)
{
	int i = 0;
	MINODE *temp_mip;
	for (i; i < NMINODE; i++) {  //Iterating through all inodes and writing to device
		temp_mip = &minode[i];
		if (temp_mip->refCount > 0)
		{
			iput(temp_mip);
		}
	}
	exit(0);
}
/*<----end of quit---->*/

/******************************************************************
** _                          MAIN                            _ **
******************************************************************/

main(int argc, char *argv[])
{
	char *remTok;	//Remaining token

	if (argc > 1)
		disk = argv[1];
	else
	{
		printf("Usage: ./a.out diskname\n");
		return;
	}

	if ((dev = fd = open(disk, O_RDWR)) < 0)
		printf("Open %s failed\n", disk);
	print_line();
	printf("-DEV: %d\n-FD: %d\n", dev, fd);

	get_block(fd, 1, buf);
	sp = (SUPER *)buf;

	if (sp->s_magic != 0xEF53)
	{
		sys_msg('X', "ERROR: Disk is not a EXT2FS type.");
		return;
	}
	sys_msg('*', "Disk is a EXT2FS type.");
	ninodes = sp->s_inodes_count;
	nblocks = sp->s_blocks_count;

	get_block(fd, 2, buf);
	gp = (GD *)buf;

	bmap = gp->bg_block_bitmap;
	imap = gp->bg_inode_bitmap;
	iblock = gp->bg_inode_table;

	//print vals
	printf("-ninodes: %d\n-iblock: %d\n-nblocks: %d\n-bmap: %d\n-imap: %d\n",
		ninodes, nblocks, bmap, imap, iblock);

	init();
	mount_root(disk);

	running = &proc[0];
	running->cwd = iget(dev, 2);
	print_line();
	while (1)
	{
		int i = -1;
		menu();
		fgets(line, 512, stdin);
		line[strlen(line) - 1] = 0;

		clear();	//Clears console screen
					/*Tokenise input*/
		char *token = strtok(line, " ");	//Retrieves token
		strcpy(cmd, token);					//Sets cmd = token
		remTok = strtok(NULL, " ");		//Stores rest
		i = lookup(cmd);					//Searches cmd and converts to index (i)
		if (!remTok)					//if remaining token is empty, pathname does not exist
			strcpy(pathname, "");			//Sets pathname empty
		else							//if remaining token not empty, pathname exists
			strcpy(pathname, remTok);		//Sets pathname with remaining token

		remTok = strtok(NULL, "");		//Stores remaining token

		if (!remTok)					//if remaining token is empty, parameter does not exist
			strcpy(parameter, "");			//Sets parameter empty
		else							//if remaining token not empty, parameter exists
			strcpy(parameter, remTok);		//Sets parameter with remaining token

		if (i != -1)
		{	//command is valid
			success_msg(0);
			switch (i)
			{	//Executes cmd
			case 0: //ls
				my_ls();
				break;
			case 1:   //cd
				chdir();
				break;
			case 2:   //pwd
				pwd();
				break;
			case 3:   //mkdir
				exec_mkdir();
				break;
			case 4:   //rmdir
				my_rmdir();
				break;
			case 5:   //creat
				my_creat();
				break;
			case 6:	  //rm (Same functionality as unlink)
				unlink();
				break;
			case 7:   //touch
				touch();
				break;
			case 8:   //link
				my_link();
				break;
			case 9:   //symlink
				symlink();
				break;
			case 10:   //unlink
				unlink();
				break;
			case 11:  //chmod
				my_chmod();
				break;
			case 12:  //stats
				my_stat();
				break;
			case 13:  //quit
				exit(0);
				my_quit();
				return 0;
			}
			success_msg(1);
		}
		else
			success_msg(-1);
	}
	return 0; //Ends program
}
