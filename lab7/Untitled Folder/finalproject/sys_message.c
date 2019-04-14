#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

void menu(void)
{
	//printf("\n                               - CMD MENU -                               ");
	putchar('\n');
	sys_msg('!', "Displaying CMD menu...");
	printf("  [ls|cd|pwd|mkdir|rmdir|creat|rm|touch|link|symlink|unlink|chmod|stat|quit]");
	printf("\n\n> ");
}

void print_line(void)
{
	printf("\n--------------------------\n");
}

void sys_msg(char symbol, char* msg)
{
	printf("[%c]-\"%s\"\n", symbol, msg);
}

void success_msg(int mode)
{
	if (mode == 0) {
		printf("[~]-\"Executing cmd \'%s\'...\"\n", cmd);
		if (!strcmp(pathname, "") == 0)
			printf("[~]-\"Processing pathname \'%s\'...\"\n", pathname);
		if (!strcmp(parameter, "") == 0)
			printf("[~]-\"Processing parameter \'%s\'...\"\n", parameter);
		printf("\n");
	}
	else if (mode == 1) {
		printf("\n");
		printf("[+]-\"Successfully executed \'%s\'.\"\n", cmd);
		print_line();
	}
	else
	{
		printf("[!]-\"ERROR: \'%s\' is not a valid command.\"\n", cmd);
		print_line();
	}
}
