#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

char *home;

int main(int filename, char *argv[], char *envp[])
{
	char input[1024], *cmd[1024];
	int x = 0, y = 1;
	home = getenv("HOME");
	printf("Type 'help' for list of basic commands.\n");
	printf("\n$Enter_Cmd> ");
	while (fgets(input, 1024, stdin) != NULL)
	{
		if (strcmp(input, "") != 0 && strcmp(input, "\n") != 0)
		{
			cmd[0] = strtok(input, "|");
			while (cmd[y] = strtok(0, "|")) {
				y++;
			}

			if (y == 1)
				computeCmd(cmd[0]);	//Computed simple commands
			else
				for (x = 0; x < y - 1; x++)
					pipeCmd(cmd[x], cmd[x + 1]); // Pipe Commands (Takes 2 Commands)

		}
		memset(input, 0, sizeof(input));
		y = 1;
		x = 0;
		printf("$Enter_Cmd> ");
	}
	return 0;
}

void pipeCmd(char cmd1[512], char cmd2[512])
{
	char *cmdArg1[512], *cmdArg2[512], *cmd, *arg;
	int procDec[2], pid, x = 1, ret1, ret2, compRed1 = 0, tempRed1 = 0, compRed2 = 0, tempRed2 = 0;

	cmd = strtok(cmd1, " ");
	cmdArg1[0] = cmd;
	arg = strtok(0, " ");



	while (arg)
	{
		if (arg[strlen(arg) - 1] == '\n')
			arg[strlen(arg) - 1] = '\0';
		cmdArg1[x] = arg;

		if ((strcmp(arg, "<") == 0) || (strcmp(arg, ">") == 0) || (strcmp(arg, ">>") == 0)) {	//Does redirect for arg1
			tempRed1 = x;
			if (strcmp(arg, "<") == 0)	//In
				compRed1 = 1;
			else if (strcmp(arg, ">") == 0)	//Out
				compRed1 = 2;
			else if (strcmp(arg, ">>") == 0)	//Append
				compRed1 = 3;
		}
		arg = strtok(0, " ");
		x++;
	}

	if (!tempRed1) {

		cmdArg1[x] = NULL;

	}
	else
	{
		cmdArg1[tempRed1] = NULL;

	}

	cmd = strtok(cmd2, " ");
	cmdArg2[0] = cmd;
	arg = strtok(0, " ");
	x = 1;
	while (arg)
	{
		if (arg[strlen(arg) - 1] == '\n')
			arg[strlen(arg) - 1] = '\0';
		cmdArg2[x] = arg;

		if ((strcmp(arg, "<") == 0) || (strcmp(arg, ">") == 0) || (strcmp(arg, ">>") == 0)) {	//Redirect for arg2
			tempRed2 = x;
			if (strcmp(arg, "<") == 0)	//In
				compRed2 = 1;
			else if (strcmp(arg, ">") == 0)	//Out
				compRed2 = 2;
			else if (strcmp(arg, ">>") == 0)	//Append
				compRed2 = 3;
		}
		arg = strtok(0, " ");
		x++;
	}
	if (!compRed2)
		cmdArg2[x] = NULL;
	else
		cmdArg2[tempRed2] = NULL;

	//Exec commands
	pipe(procDec);
	if (fork() == 0)
	{
		dup2(procDec[1], 1);
		close(procDec[0]);
		close(procDec[1]);
		if (compRed1 == 1)
		{	//In
			close(0);
			open(cmdArg1[x - 1], O_RDONLY);
		}
		else if (compRed1 == 2 || (compRed1 == 3)) {
			close(1);

			 if (compRed1 == 2)	//Out
				open(cmdArg1[x - 1], O_WRONLY | O_CREAT, 0644);
			else if (compRed1 == 3)	//Append
				open(cmdArg1[x - 1], O_WRONLY | O_APPEND);
		}
		execvp(cmdArg1[0], cmdArg1);
	}
	if (fork() == 0)
	{
		dup2(procDec[0], 0);
		close(procDec[0]);
		close(procDec[1]);
		if (compRed2 == 1)
		{	//In
			close(0);
			open(cmdArg2[x - 1], O_RDONLY);
		}
		else if (compRed2 == 2 || (compRed2 == 3)) {
			close(1);

			if (compRed2 == 2)	//Out
				open(cmdArg2[x - 1], O_WRONLY | O_CREAT, 0644);
			else if (compRed2 == 3)	//Append
				open(cmdArg2[x - 1], O_WRONLY | O_APPEND);
		}
		execvp(cmdArg2[0], cmdArg2);
	}
	close(procDec[0]);
	close(procDec[1]);
	wait(&ret1);
	wait(&ret2);
	printf("cmd1 exit: %d / cmd2 exit: %d\n", ret1, ret2);
}

void computeCmd(char input[512])
{
	char *args[512], *cmd, *arg;
	int pid, status, handle = 0, rediri = 0, x = 1;
	input[strlen(input) - 1] = 0;

	cmd = strtok(input, " ");
	arg = strtok(0, " ");

	if (!defaultCmd(cmd, arg))
	{
		args[0] = cmd;
		while (arg != NULL)
		{
			args[x] = arg;
			if (strcmp(arg, "<") == 0)
			{
				rediri = x;
				handle = 1;
			}
			else if (strcmp(arg, ">") == 0)
			{
				rediri = x;
				handle = 2;
			}
			else if (strcmp(arg, ">>") == 0)
			{
				rediri = x;
				handle = 3;
			}
			arg = strtok(0, " ");
			x++;
		}


		if (!rediri) {
			args[x] = NULL;
		}
		else
		{
			args[rediri] = NULL;
		}
		


		pid = fork();
		if (!pid)
		{
			if (handle == 1)
			{	//In
				close(0);
				open(args[x - 1], O_RDONLY);
			}
			else if (handle == 2)
			{	//Out
				close(1);
				open(args[x - 1], O_WRONLY | O_CREAT, 0644);
			}
			else if (handle == 3)
			{	//Append
				close(1);
				open(args[x - 1], O_WRONLY | O_APPEND);
			}
			execvp(cmd, args);
			printf("Did not execute: %s", cmd);
		}
		else if (pid)
		{
			pid = wait(&status);
			printf("child exit: %d\n", status);
		}
	}
}

int defaultCmd(char *cmd, char *arg)
{
	int done = 0;
	if (strcmp(cmd, "cd") == 0)
	{
		done = 1;
		if (!arg)
		{
			if (home)
			{
				chdir(home);
			}
		}
		else
			chdir(arg);
	}
	else if (strcmp(cmd, "pwd") == 0)
	{
		printf("%s\n", get_current_dir_name());
		done = 1;
	}
	else if (strcmp(cmd, "exit") == 0)
	{
		exit(1);
		done = 1;
	}
	else if (strcmp(cmd, "help") == 0)
	{
		printf("Basic Default Commands:\n\t cd \"__\" - change directory\n\t pwd - prints current directory\n\texit - exits terminal\n");
		done = 1;
	}
	return done;
}
