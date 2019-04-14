#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "util.c"
#include "iget_iput_getino.c"

char *disk = "mydisk";
char line[128], cmd[256], pathname[256], parameter[256];
char buf[BLKSIZE];
int my_fd, iblk;

//Initializer
int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = 0;
    p->cwd = 0;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}

void menu ()
{
  printf("[ls|cd|pwd|mkdir|-rmdir|-creat|-rm|-link|-symlink|-unlink|-chmod|-stat|quit]: ");
}

int lookup(char *cmd)
{
  char *arr[100] = {"ls", "cd", "pwd", "mkdir", "rmdir", "creat", "rm", "link", "symlink", "unlink", "chmod", "stat", "quit"};
  for (int i = 0; i < 13; i++)
  {
    if (!strcmp(arr[i], cmd))
    {
      return i;
    }
  }
  printf("\nCommand does not exist, please enter a new one\n");
}

int mount_root()
{
  MNTABLE mntable, *mntPtr;
  printf("mount_root()\n");
  root = iget(dev, 2);
  root->mounted = 1;
  root->mptr = &mntable;

  mntPtr = &mntable;
  mntPtr->dev = dev;
  mntPtr->ninodes = ninodes;
  mntPtr->nblocks = nblocks;
  mntPtr->bmap = bmap;
  mntPtr->imap = imap;
  mntPtr->iblk = iblk;
  mntPtr->mntDirPtr = root;
  strcpy(mntPtr->devName, "mydisk");
  strcpy(mntPtr->mntName, "/");
}

/************* LEVEL 1 FUNCTIONS START **********************/
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
     putchar('\t');

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

int my_ls()
{	//Figures whether to call ls_dir or ls_file
  MINODE *mip;	//memory inode pointer
  u16 mode;
  int dev, ino;

  if (pathname[0] == 0)	//If not given a pathname value is 0 and calls ls_dir on cwd
    ls_dir(running->cwd);
  else{	//Calls ls_file
    dev = root->dev;	// set device to point at root's device
    printf("%s\n", pathname);
    ino = getino(&dev, pathname); // calling getino on the device set in previous line and using pathname
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

int chdir()
{//treat just like ls at first
	int tempIno;
	MINODE *mip;

	if (strcmp(pathname, "") != 0){


		if (pathname[0] == '/')
		{//Is it absolute
			dev = root->dev;
		}
		else
		{//not absolute
			dev = running->cwd->dev;
		}

		//Let's get the ino.
		tempIno = getino(&dev, pathname);

		if (tempIno == 0)
		{
			printf("This pathname: %s does not exist.\n", pathname);
			return;
		}

		mip = iget(dev, tempIno);

		if (S_ISDIR(mip->INODE.i_mode))
		{
			iput(running->cwd);
			running->cwd = mip;
		}
		else
		{
			iput(mip);
			printf("Cannot CD into a file...\n");
		}
	}
	else
	{
		iput(running->cwd);
		running->cwd = iget(dev,2);
	}
}


/*beginning of pwd*/
print_pwd(int ino, MINODE *mip)
{
  char *cp;
  char buf[BLKSIZE];

  ip = &(mip->INODE);

  for (int i = 0; i < 12; i++)
  {
    if (ip->i_block[i] == 0)
      return;

    get_block(dev, ip->i_block[i], buf);

    dp = (DIR *)buf;
    cp = dp;
    while (cp < &buf[1024])
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

void rpwd(MINODE *mip)
{
  if (mip == root)
  return;
  int ino;
  MINODE *temp_mip;
  char *parent = "..";
  ino = getino(&mip->dev, parent);
  temp_mip = iget(mip->dev, ino);
  rpwd(temp_mip);
  print_pwd(mip->ino, temp_mip);
  iput(temp_mip);
}

void pwd(void)
{
  MINODE *mip;
  int i = 0;
  if (running->cwd->ino == 2) //checks if cwd is root
  {
    printf("/\n");
    return;
  }
  mip = running->cwd;
  rpwd(mip);
  printf("\n");
}



int makedir()
{
	MINODE *mip, *pip;
	char dirN[256], basN[256];
	char *parent, *child;
	if (pathname[0] == "/")
	{
		mip = root;
		dev = root->dev;
	}
	else
	{
		mip = running->cwd;
		dev = running->cwd->dev;
	}

	strcpy(dirN, pathname);
	strcpy(basN, pathname);
	parent = dirname(dirN);
	child = basename(basN);

	//Grab MINODE of parent.
	printf("dev in make_dir %d\n", dev);
	int pino = getino(&dev, parent);
	pip = iget(dev, pino);


	if(!S_ISDIR(pip->INODE.i_mode))
	{//if file
		printf("you cannot make a dir in a file\n");
		return -1;
	}
	if(getino(&dev, pathname))
	{//if exists
		printf("this already exists\n");
		return -1;
	}

	mymkdir(pip, child);

	//Touch time
	pip->INODE.i_atime = time(0L);
	pip->INODE.i_links_count += 1;
	pip->dirty = 1;
	pip->INODE.i_mode = 0040000;

	iput(pip);

}


int creat_file()
{
	MINODE *mip, *pip;
	int tempIno;
	char dirN[256], basN[256];
	char *parent, *child;
	if (pathname[0] == "/")
	{
		//Its absolute
		mip = root;
		dev = root->dev;
	}
	else
	{
		mip = running->cwd;
		dev = running->cwd->dev;
	}


	parent = dirname(pathname);
	child = basename(pathname);

	//Grab MINODE of parent.
	int pino = getino(&dev, parent);
	pip = iget(dev, pino);

	my_creat(pip, child);

	//Touch time
	pip->INODE.i_atime = time(0L);
	pip->INODE.i_links_count = 1;
	pip->dirty = 1;

	iput(pip);
}

int rmdir()
{
	int temp_ino, par_ino;
	MINODE *mip, *pip;
	char buff[BLKSIZE];
	char *cp;


	//Ino and MINODE of pathname
	temp_ino = getino(&dev, pathname);
	mip = iget(dev, temp_ino);



	if (temp_ino == 0)
	{
		printf("Not valid pathname\n");
	}

	if (S_ISREG(mip->INODE.i_mode))
	{
		printf("Not directory\n");
		return;
	}
	get_block(dev, mip->INODE.i_block[0], buff);

	dp = (DIR *)buff;
	cp = buff;
	cp += dp->rec_len;
	dp = (DIR *)cp;

	par_ino = dp->inode;
	if (dp->rec_len != 1012)
	{
		printf("Directory is not empty\n");
		return;
	}

	truncate(mip);
	idealloc(dev, mip->ino);
	//put minode back
	iput(mip);
	pip = iget(dev, par_ino);

	rm_child(pip, basename(pathname));

	//decrement this
	pip->INODE.i_links_count--;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);
	return;


}

int rm()
{
	int temp_ino, par_ino;
	MINODE *mip, *pip;
	char buff[BLKSIZE];
	char *cp;

  if(pathname[0] == 0)
  {
    printf("ERROR: must include a filename\n");
    return;
  }
	//Ino and MINODE of pathname
	temp_ino = getino(&dev, pathname);
	mip = iget(dev, temp_ino);

  printf("test: %d\n", mip->ino);

	if (temp_ino == 0)
	{
		printf("Not valid pathname\n");
	}

	if (S_ISDIR(mip->INODE.i_mode))
	{
		printf("Not directory\n");
		return;
	}

	par_ino = mip->INODE.i_block[0];

  printf("breaks after here\n");
	truncate(mip);
	idealloc(dev, mip->ino);
	//put minode back
	iput(mip);
	pip = iget(dev, par_ino);


	//decrement this
	pip->INODE.i_links_count--;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	iput(pip);
	return;


}

int hard_link()
{
	//Get INO of pathname.
	int temp_ino, par_ino;
	MINODE *mip, *tip;
	char parent[64], child[64];
	char *basen, *dirn;

	temp_ino = getino(&dev, pathname);
	printf("Received INO of %d\n", temp_ino);

	if (temp_ino == 0)
	{
		printf("This pathname doesn't exist...\n");
		return;
	}

	//Get the MINODE corresponding to that ino.
	mip = iget(dev, temp_ino);

	//Quick check for smartness.
	if (S_ISDIR(mip->INODE.i_mode))
	{
		printf("Can't link a directory...\n");
		return;
	}

	//Dissect parameter into base and dir
	strcpy(parent, parameter);
	strcpy(child, parameter);
	//Example: /a/b/c
	basen = dirname(parent); // /a/b
	dirn = basename(child); // cs


	//Check /x/y exists
	par_ino = getino(&dev, basen);
	if (par_ino == 0)
	{
		printf("Parent doesn't exist\n");
		return;
	}

	tip = iget(dev, par_ino);
	enter_name(tip, temp_ino, dirn);
	tip->INODE.i_links_count++;

	iput(tip);
	iput(mip);
}


int unlink()
{
	int path_ino, par_ino;
	char *basen, *dirn, temppath1[128], temppath2[128];
	MINODE *mip, *pip;

	path_ino = getino(&dev, pathname);
	mip = iget(dev, path_ino);

	//Verify it's a file
	if (S_ISDIR(mip->INODE.i_mode))
	{
		printf("Can't unlink a directory...\n");
		return;
	}

	mip->INODE.i_links_count--;

	//This is a check to see if the file has zero connections
	//Essentially a delete.
	if (mip->INODE.i_links_count == 0)
	{
		truncate(mip);
		idealloc(dev, mip->ino);
	}


	strcpy(temppath1,pathname);
	strcpy(temppath2,pathname);
	basen = basename(temppath1);
	dirn = dirname(temppath2);

	printf("Basename: %s\n", basen);

	par_ino = getino(&dev, dirn);
	pip = iget(dev, par_ino);

	rm_child(pip, basen);
	iput(pip);
	iput(mip);
}

int symlink()
{
	//assuming oldname (pathname) only has 60 chars...
	char oldname[84],temppath2[128], temppath1[128];
	char *basen, *dirn, *write_in;
	int old_ino, par_ino;
	MINODE *pip, *mip;
	//This is the pathname, i.e /a/b/c
	strncpy (oldname, pathname, 60);

	//Let's get base and dir
	strcpy(temppath1, parameter);
	strcpy(temppath2, parameter);
	dirn = dirname(temppath1);
	basen = basename(temppath2);

	old_ino = getino(&dev, parameter);
	if (old_ino != 0)
	{
		printf("This file already exists\n");
		return;
	}
	//This means they are the same and not absolute.
	printf("Dirn: %s basen: %s param: %s\n",dirn, basen, parameter);
	if (strcmp(basen, parameter) == 0)
	{
		par_ino = running->cwd->ino;
		pip = iget(dev, par_ino);
		my_creat(pip, basen);
	}
	else
	{
		par_ino = getino(&dev, dirn);

		pip = iget(dev, par_ino);
		printf("\n\npar_ino:%d dirn: %s\n", par_ino, dirn);
		my_creat(pip, basen);
	}

	//Write oldname into that new file.
	old_ino = getino(&dev, parameter);
	mip = iget(dev, old_ino);
	mip->INODE.i_mode = 0xA000;
	//write_in = (char*)mip->INODE.i_block;
	strcpy(mip->INODE.i_block, pathname);
	iput(mip);
}


int my_stat() {
	int tempIno, i = 0;
	char *cp;
	int dev = running->cwd->dev;
	MINODE *mip = running->cwd, *tip;
	char buff[1024], buff2[1024];

	if (strcmp(pathname, "") != 0)
	{
		if (pathname[0] == '/')
		{
			dev = root->dev;
		}

		tempIno = getino(&dev, pathname);

		mip = iget(dev, tempIno);
	}
	else
	{
		tempIno = running->cwd->ino;
		mip = running->cwd;
	}
	printf("Name\t Rec_Len Ino\n SIZE");
	while (mip->INODE.i_block[i])
	{
		get_block(dev, mip->INODE.i_block[i], buff);
		cp = buff;
		dp = (DIR *)buff;
		while (cp < &buff[1024])
		{
			tip = iget(dev, dp->inode);
			strncpy(buff2, dp->name, dp->name_len);
			buff2[dp->name_len] = 0;
			printf("%s\t %d\t %d\t %d\n", buff2, dp->rec_len,dp->inode, tip->INODE.i_size);
			iput(tip);
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

int readlink()
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

	printf("%s\n",mip->INODE.i_block);

	printf("\n");

}


/*void my_chmod()
{
	printf("Do we enter the function\n");
	int temp_ino;
	MINODE *mip;
	sscanf(pathname, "%o", &mode);
	if (parameter[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		//Else. Check where we are at, and assign that device.
		dev = running->cwd->dev;
	}
	printf("Dec %d, oct %o\n", mode, mode);
	temp_ino = getino(&dev, parameter);
	mip = iget(dev, temp_ino);
	mip->INODE.i_mode = mode;

	iput(mip);

}*/


/*beginning of quit*/
int quit(void)
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
/*end of quit*/


/************* LEVEL 1 FUNCTIONS END **********************/



main(int argc, char *argv[ ])
{
  char *hold;
	//Check for the input
	if (argc > 1)
		disk = argv[1];
  else
  {
    printf("Usage: ./a.out diskname");
    return;
  }

	if ((dev = fd = open(disk, O_RDWR)) < 0)
	{
		printf("Open %s failed\n", disk);
	}

	printf("DEV: %d\t FD: %d\n", dev, fd);

	printf("Quick check if disk is EXT2...\n");


	get_block(fd, 1, buf);
	sp = (SUPER *)buf;

	if(sp->s_magic != 0xEF53)
	{
		printf("%x %s is not an EXT2 FS\n", sp->s_magic, dev);
		exit(1);
	}
        printf("OK\n");
	ninodes = sp->s_inodes_count;
	nblocks = sp->s_blocks_count;

	get_block(fd, 2, buf);
	gp = (GD *)buf;

	bmap = gp->bg_block_bitmap;
	imap = gp->bg_inode_bitmap;
	iblock = gp->bg_inode_table;

	//print vals
	printf("ninodes: %d\t nblocks: %d\t bmap: %d\t imap: %d\t iblock: %d\t",
		ninodes, nblocks, bmap, imap, iblock);

  init();

  mount_root(disk);

  running = &proc[0];
  running->cwd = iget(dev, 2);

  while (1)
  {
    menu();
    fgets(line, 512, stdin);
		line[strlen(line)-1] = 0;

    char *token = strtok(line, " ");

		strcpy(cmd,token);

    hold = strtok(NULL, " ");

    int i = lookup(cmd);
    if (!hold)
		{
			strcpy(pathname, "");
		}
		else
		{
			strcpy(pathname, hold);
		}

		hold = strtok(NULL, "");

		if (hold)
			strcpy(parameter, hold);

		if (strcmp(parameter, "") == 0)
			printf("cmd: %s\t pathname: %s\n\n", cmd, pathname);

		else

			printf("cmd: %s\t pathname: %s\t parameter: %s\n\n", cmd, pathname, parameter);

      printf("value of i: %d\n", i);

    switch (i)
    {
      case 0: //ls
        my_ls();
        break;
      case 1: //cd
      chdir();
        break;
      case 2: //pwd
        pwd();
        break;
      case 3: //mkdir
        makedir();
        break;
      case 4: //rmdir
        rmdir();
        break;
      case 5: //creat
        creat_file();
        break;
      case 6: //rm
        rm();
        break;
      case 7: //link
        break;
      case 8: //symlink
        break;
      case 9: //unlink
        break;
      case 10:  //chmod
        break;
      case 11:  //stats
        break;
      case 12: //quit
        quit();
        return 0;
    }
  }


  return 0;
}
