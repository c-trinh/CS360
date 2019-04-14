#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef struct NODE {
	char name[64];
	char type;
	struct NODE *childPtr, *siblingPtr, *parentPtr, *prevDir;
} NODE;

typedef int bool;
enum { true, false };

NODE *root, *cwd;                             /* root and CWD pointers */
char argument[64];
char line[128];                               /* user input line */
char command[16], pathname[64];               /* user inputs */
char dirname[64], basename[64];               /* string holders */
											  //(Others as needed)

char *cmd[] = { "menu", "mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
"reload", "save", "quit", 0 };

//int (*fptr[ ])(char *) = {(int(*)())menu, mkdir,rmdir,ls, cd, pwd,creat,rm};

void initialize()
{
	if (root == 0)
	{
		//printf("\nIn init()\n");
		root = (NODE *)malloc(sizeof(NODE));
		strcpy(root->name, "/");
		root->type = 'D';
		root->childPtr = 0;
		root->siblingPtr = 0;
		root->parentPtr = root;
		cwd = root;
	}
}

int findCommand(char* command)
{
	int i = 0;
	//printf("command?: %s, len: %d\n", command, strlen(command));
	while (cmd[i]) {
		if (strcmp(command, cmd[i]) == 0)
			return i;
		i++;
	}
	return -1;
}

void menu()
{
	printf("menu\nmkdir\nrmdir\nls\ncd\npwd\ncreat\nrm\nreload\nsave\nquit\n\n");
}

void mkdirHead(char *arg) {
	NODE *temp = (NODE *)malloc(sizeof(NODE));
	temp->type = 'D';
	strcpy(temp->name, arg);
	temp->childPtr = 0;
	temp->siblingPtr = 0;
	temp->parentPtr = cwd;
	cwd->childPtr = temp;
	NODE *first = cwd->childPtr;
	printf("Directory Created.\nDirectory name: %s\n\n", first->name);
}

void mkdirAppend(char *arg) {
	//prevDir = cwd->childPtr;
	NODE *temp = cwd->childPtr;
	while (temp->siblingPtr != 0)
		temp = temp->siblingPtr;
	NODE *target = (NODE *)malloc(sizeof(NODE));
	target->type = 'D';
	strcpy(target->name, arg);
	target->childPtr = 0;
	target->siblingPtr = 0;
	target->parentPtr = cwd;
	temp->siblingPtr = target;
	NODE *childName = temp->siblingPtr;
	printf("Directory Created.\nDiretory name: %s\n\n", childName->name);
}

void mkdir(char *arg)
{
	if (cwd->childPtr == 0)
		mkdirHead(arg);
	else
		mkdirAppend(arg);
}

void rmdir(char *arg) {
	char *tempStr[100];
	strcpy(tempStr, arg);
	if (root->childPtr != 0) {
		NODE *last = 0, *temp = root->childPtr;
		while (strcmp(tempStr, temp->name) != 0) {	//Searches for name
			last = temp;
			temp = temp->siblingPtr;
		}
		if (temp->type == 'D' && temp->childPtr == 0)
		{	//Found
			if (temp->parentPtr->childPtr == temp)
			{
				if (temp->siblingPtr == 0)	//Removes first
					temp->parentPtr->childPtr = 0;
				else
					temp->parentPtr->childPtr = temp->siblingPtr;
			}
			else
			{	//Removes anything after first
				last->siblingPtr = temp->siblingPtr;
				temp->siblingPtr = 0;
			}
			free(temp);
		}
	}
}

void ls() {
	if (cwd->childPtr == 0)
	{
		printf("Directory is Empty.\n\n");
		return;
	}
	NODE *temp = cwd->childPtr;
	printf("Name\tType\n");
	while (temp != 0)
	{
		printf("%s\t%c\n", temp->name, temp->type);
		temp = temp->siblingPtr;
	}
	printf("\n");
}

void cd(char *arg)
{
	char *na[100];
	strcpy(na, arg);
	NODE *previous = 0;
	NODE *temp = root->childPtr;
	if (strcmp(na, "..") != 0) {
		while (strcmp(na, temp->name) != 0) {
			previous = temp;
			temp = temp->siblingPtr;
		}
		if (strcmp(na, temp->name) != 0) {
			printf("Directory does not exist.\n");
			return;
		}
		else {
			cwd = temp;
			printf("Current Directory: %s\n\n", temp->name);
		}
	}
	else {
		cwd = cwd->parentPtr;
		printf("Current directory: %s\n\n", cwd->name);
	}
}


void rpwd(NODE *arg)
{
	if (arg->parentPtr == arg)
	{
		printf("/");
		return;
	}
	else
	{
		rpwd(arg->parentPtr);
		printf("%s/", arg->name);
	}
	printf("\n\n");
}

void pwd()
{
	rpwd(cwd);
}

void creatHead(char *arg) {
	NODE *temp = (NODE *)malloc(sizeof(NODE));
	temp->type = 'F';
	strcpy(temp->name, arg);
	temp->childPtr = 0;
	temp->siblingPtr = 0;
	temp->parentPtr = cwd;
	cwd->childPtr = temp;
}

void creatAppend(char *arg) {
	NODE *temp = cwd->childPtr;
	if (temp->type == 'F' && strcmp(temp->name, arg) == 0)
	{
		printf("File name already exists.\n");
		return;
	}

	while (temp->siblingPtr != 0)
	{
		temp = temp->siblingPtr;
		if (temp->type == 'F' && strcmp(temp->name, arg) == 0)
		{
			printf("File name already exists.\n");
			return;
		}
	}
	NODE *target = (NODE *)malloc(sizeof(NODE));
	target->type = 'F';
	strcpy(target->name, arg);
	target->childPtr = 0;
	target->siblingPtr = 0;
	target->parentPtr = cwd;
	temp->siblingPtr = target;
}

void creat(char *arg) //Creates a new Dir node with the name of the token
{
	if (cwd->childPtr == 0)
		creatHead(arg);
	else
		creatAppend(arg);
}

int rm(char *arg) {
	char *tempStr[64];
	strcpy(tempStr, arg);
	if (root->childPtr != 0) {
		NODE *previous = 0;
		NODE *temp = root->childPtr;
		while (strcmp(tempStr, temp->name) != 0) {

			previous = temp;
			temp = temp->siblingPtr;
		}
		if (temp->type == 'F')
		{
			if (temp->parentPtr->childPtr == temp)
				if (temp->siblingPtr == 0)	//Removes first
					temp->parentPtr->childPtr = 0;
				else
					temp->parentPtr->childPtr = temp->siblingPtr;
			else
			{	//Removes everything after first
				previous->siblingPtr = temp->siblingPtr;
				temp->siblingPtr = 0;
			}
			free(temp);
		}
	}
	else
		printf("Directory is Empty.\n");
}
void reload(char *filename)
{
	//Splice pwd, create firectories after each dir
	printf("Reloaded.\n");
}

void save(char *filename)
{
	FILE *fp = fopen("myfile", "w+");           // open a FILE stream for WRITE
	NODE *temp = root;
	while (temp == NULL) {
		fprintf(fp, "%c, %s", 'D', "namestring\n");  // print a line to file by FORMAT
													 //Print name, type, pwd
	}
	fclose(fp);                                 // close FILE stream when done
}

void quit(filename)
{
	save(filename); //Deconstruct
}

bool contains(char* String, char c)
{
	int size = strlen(String) - 1;
	for (int x = 0; x < size; x++) {
		if (String[x] == c) {
			return true;
		}
	}
	return false;
}

void seperate(char * input) {
	//char str[] ="- This, a sample string.";
	strtok(input, "\n");
	if (contains(input, ' ') == true) {
		char * pch;
		//printf ("Splitting string \"%s\" into tokens:\n",str);
		pch = strtok(input, " ");
		strcpy(command, pch);
		if (pch != NULL)
		{
			//printf ("%s\n",pch);
			pch = strtok(NULL, " ,-");
			strcpy(argument, pch);
		}
		//printf("ARG: %s\n", argument);
	}
}

int main()
{
	initialize();      /* initialize / node of the file system tree */
	int ID = -1;
	while (ID != 10) {
		printf(">>input a command: ");
		//scanf("%s", command);
		fgets(command, 256, stdin);
		seperate(command);
		//printf("COMMAND: %s\n", command);
		printf("\n");
		ID = findCommand(command);
		switch (ID) {
		case 0: menu();                 break;
		case 1: mkdir(argument);        break;
		case 2: rmdir(argument);        break;
		case 3: ls();                   break;
		case 4: cd(argument);           break;
		case 5: pwd(cwd);               break;
		case 6: creat(argument);        break;
		case 7: rm(argument);           break;
		case 8: reload(argument);       break;
		case 9: save(argument);         break;
		case 10: quit(argument);        break;
		}
	}
	return 0;
}
