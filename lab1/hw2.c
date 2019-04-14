#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void myprintf(char* format, ...);

// PART 1
/*2-2. Write YOUR ONW fucntions

int  printd(int x) which prints an integer (x may be negative!!!)
int  printo(u32 x) which prints x in Octal (start with 0  )
int  printx(u32 x) which prints x in HEX   (start with 0x )*/

//typedef unsigned int u32;
//typedef unsigned char u8;

char *ctable = "0123456789ABCDEF";
uint32_t INT_BASE = 10, OCTAL_BASE = 8, HEX_BASE = 16; // for decimal numbers
uint16_t UNSIGNED_BASE = 10;
char buf[512];
int fd;
struct partition *p;

struct partition {
	uint8_t drive;
	uint8_t head;
	uint8_t sector;
	uint8_t cylinder;

	uint8_t sys_type;

	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cylinder;

	uint32_t start_sector;
	uint32_t nr_sectors;
};

void rpu(uint32_t x, uint32_t BASE)
{
	char c;
	if (x)
	{
		c = ctable[x % BASE];
		rpu(x / BASE, BASE);
		putchar(c);
	}
}

void prints(char *x)
{
	while (*x != '\0')
	{
		putchar(*x);
		x++;
	}
}

void printd(int x)
{
	if (x < 0)
	{
		x = -x;
		putchar('-');
	}
	(x == 0) ? putchar('0') : rpu(x, INT_BASE);
	putchar(' ');
}


void printu(uint16_t x)
{
	(x == 0) ? putchar('0') : rpu2(x, UNSIGNED_BASE);
	putchar(' ');
}

void rpu2(uint16_t x, uint16_t BASE) {
	char c;
	if (x) {
		c = ctable[x % BASE];
		rpu2(x / BASE, BASE);
		putchar(c);
	}
}

void printo(uint32_t x)
{
	(x == 0) ? putchar('0') : rpu2(x, OCTAL_BASE);
	putchar(' ');
}

void printx(uint32_t x)
{
	putchar('0');
	putchar('x');
	(x == 0) ? putchar('0') : rpu2(x, HEX_BASE);
	putchar(' ');
}

void myprintf(char* format, ...)
{
	char *cp = format;
	uint16_t *ip = (uint16_t *)&format + 1;
	uint32_t *up = (uint32_t *)&format + 1;
	while (*cp)
	{
		if (*cp != '%')
		{
			putchar(*cp);
			if (*cp == '\n')
				putchar('\r');
			cp++; continue;
		}
		cp++;
		switch (*cp)
		{
		case 'c':
			putchar(*up);
			break;
		case 's':
			prints((char *)*up);
			break;
		case 'u':
			printu(*up);
			break;
		case 'd':
			printd(*up);
			break;
		case 'o':
			printo(*up);
			break;
		case 'x':
			printx(*up);
			break;
		}
		cp++;
		up++;
		ip++;
	}
}

int main(int argc, char *argv[], char *env[])
{
	int sector, origin;
	int fd = open("vdisk", O_RDONLY);          // open disk iamge file for READ

	int r = read(fd, buf, 512);                // read FIRST 512 bytes into buf[ ]
	int x = 0, a = 0;

	myprintf("1.)\ncha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n",
		'A', "this is a test", 100, 100, 100, -100);

	myprintf("\n2.)\nFD: %d\n", fd);
	if (fd > 1)
		myprintf("vdisk accessed.\n");
	else
		myprintf("vdisk not accessed.\n");

	//printf("argc = %d\n", 6);
	myprintf("argc = %d\n", argc);

	while (*argv != NULL)
	{
		//myprintf("argv[%d] = %s\n", a, *argv);
		myprintf("argv[ %d] = %s\n", a, *argv);
		argv++;
		a++;
	}
	while (*env != NULL)
	{
		//myprintf("argv[%d] = %s\n", a, *argv);
		myprintf("env[ %d] = %s\n", x, *env);
		env++;
		x++;
	}

	//Raw Form

	p = (struct partition *)(&buf[0x1BE]); // p points at Ptable in buf[ ]
	myprintf("\n**RAW form**\n");
	for (int x = 0; x < 4; x++) {
		//myprintf
		myprintf("%d\t", p->drive);
		myprintf("%d\t", p->head);
		myprintf("%d\t", p->sector);
		myprintf("%d\t", p->cylinder);

		myprintf("%x\t", p->sys_type);

		myprintf("%d\t", p->end_head);
		myprintf("%d\t", p->end_sector);
		myprintf("%d\t", p->end_cylinder);

		myprintf("%d\t", p->start_sector);
		myprintf("%d\n", p->nr_sectors);
		p++;
	}
	p = (struct partition *)(&buf[0x1BE]); // p points at Ptable in buf[ ]

										   //fDisk

	myprintf("\n**Linux fdisk 'p' output form**\n");
	myprintf("Start\tEnd\tSize\n");
	for (int x = 0; x < 4; x++) {
		myprintf("%d\t", p->start_sector);
		myprintf("%d\t", p->nr_sectors + p->start_sector - 1);
		myprintf("%d\n", p->nr_sectors);

		p++;
	}

	//Extended Partition

	printf("\n**Extend partitions**\n");
	p = (struct partition *)(&buf[0x1BE]) + 3;

	if (p->sys_type == 5) {

		myprintf("start sector: %d\n", p->start_sector);

		sector = p->start_sector;
		origin = p->start_sector;

		while (p->start_sector != 0) {
			p = (struct partition *)(&buf[0x1BE]);
			lseek(fd, (long)(sector * 512), 0);     // seek to sector 10          
			read(fd, buf, 512);                   	// read sector 10 into buf[ ], etc.
			
			myprintf("Local Mbr start_sector = %d partition start_sector = %d\n", sector, p->start_sector);
			myprintf("start: %d\t", sector + p->start_sector);
			myprintf("end: %d\t", (sector + p->start_sector + p->nr_sectors) - 1);
			myprintf("size: %d\n", p->nr_sectors);
			
			p++;
			
			sector = p->start_sector + origin;
			
		}
	}

	close(fd);
}
