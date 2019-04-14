// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>

#define  MAX 256

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables
char buf[MAX];
// Server initialization code:

int server_init(char *name)
{
   printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

   printf("1 : get and show server host info\n");
   hp = gethostbyname(name);
   if (hp == 0){
      printf("unknown host\n");
      exit(1);
   }
   printf("    hostname=%s  IP=%s\n",
               hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
   //  create a TCP socket by socket() syscall
   printf("2 : create a socket\n");
   mysock = socket(AF_INET, SOCK_STREAM, 0);
   if (mysock < 0){
      printf("socket call failed\n");
      exit(2);
   }

   printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = 0;   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
       printf("bind failed\n");
       exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
      printf("get socketname error\n");
      exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("6 : server is listening ....\n");
   listen(mysock, 5);
   printf("===================== init done =======================\n");

   
   
}

char *cmd[] = {  "get", "put","ls", "cd", "pwd", "mkdir", "rmdir", "rm"};

int findCmd(char *command)
{
	int i = 0;
	while (cmd[i]) {
		if (strcmp(command, cmd[i]) == 0)
			return i;
		i++;
	}
	return -1;
}
void cd(char pathname[]){ 
  char mycwd[MAX] = {'\0'};
  int success = 2;
  printf("pathname: [%s]\n", pathname);
  if (pathname[0] != '/'){ //relative
    
    getcwd(mycwd, MAX);
    if (strcmp(pathname, "..") == 0){ //go back one dir
      int start = strlen(mycwd) - 1;
      while (mycwd[start] != '/'){
        mycwd[start] = 0;
        start--;
      }
      printf("mycwd: %s\n");
    }
    else {

    strcat(mycwd, "/");
    strcat(mycwd, pathname);
    }
    success = chdir(mycwd);
  }
  else {
    success = chdir(pathname);
  }
  if (success == 0){
    sprintf(buf, "cd %s OK\n", pathname);
  }
  else {
    sprintf(buf, "cd %s FAILED\n", pathname);
  }
  n = write(client_sock, buf, MAX);
}

void pwd(){
  char tmp[MAX];
  if (getcwd(tmp, MAX) == NULL){
    
    printf("pwd FAILED\n"); 
    return;
  }
  sprintf(buf, "pwd=%s\n", tmp);
  printf("%s",buf);

  n = write(client_sock, buf, MAX);
  
  //printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, tmp);
}
void ls(char path[]){
  DIR *dirp;
  struct dirent *ent;
  char tmp[MAX];
  strcpy(tmp, path);
  if (path[0] != '/'){ //relative
    
    if (getcwd(tmp, MAX) == NULL){

      printf("ls %s failed\n", path); 
      return;
    }
  }
  if ((dirp = opendir(tmp))!= NULL){
    while((ent = readdir(dirp)) != NULL)
    {
      struct stat fileStat;
      stat(ent->d_name, &fileStat);
      
      char mode[MAX] = {0};
      strcat(mode, (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
      strcat(mode, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
      strcat(mode, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
      strcat(mode, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
      strcat(mode, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
      strcat(mode, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
      strcat(mode, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
      strcat(mode, (fileStat.st_mode & S_IROTH) ? "r" : "-");
      strcat(mode, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
      strcat(mode, (fileStat.st_mode & S_IXOTH) ? "x" : "-");
      char f_time[MAX];
      strcpy(f_time,ctime(&fileStat.st_mtime));
      f_time[strlen(f_time) - 1] = 0;
      char tempp[MAX];
      sprintf(tempp, "%s %d %s %s \n",mode, fileStat.st_size, f_time, ent->d_name);
      printf("ls_file %s\n", ent->d_name);
      strcat(buf, tempp);
      
    
    }
    n = write(client_sock, buf, MAX);
  }
  else {
    printf("ls %s FAILED\n", path);
  }

}
main(int argc, char *argv[])
{
   char *hostname;
   char line[MAX];

   if (argc < 2)
      hostname = "localhost";
   else
      hostname = argv[1];
 
   server_init(hostname); 

  //set root
  char mycwd[MAX];
  if(getcwd(mycwd, MAX) != NULL){
    chdir(mycwd);
    printf("server: chroot to %s\n", mycwd);
  }

   // Try to accept a client request
   while(1){
     printf("server: accepting new connection ....\n"); 

     // Try to accept a client connection as descriptor newsock
     length = sizeof(client_addr);
     client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
     if (client_sock < 0){
        printf("server: accept error\n");
        exit(1);
     }
     printf("server: accepted a client connection from\n");
     printf("-----------------------------------------------\n");
     printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                                        ntohs(client_addr.sin_port));
     printf("-----------------------------------------------\n");

     // Processing loop: newsock <----> client
     while(1){
       n = read(client_sock, line, MAX);
       if (n==0){
           printf("server: client died, server loops\n");
           close(client_sock);
           break;
      }
      
      // show the line string
      printf("server: read  n=%d bytes; line=[%s]\n", n, line);
      char command[MAX], pathname[MAX];
      
   


      //strcpy(cmd, &line[1]);
      sscanf(line, "%s %s", command, pathname);

      int index = findCmd(command);
      printf("index: %d\n", index);

      switch (index) {
        //case 0: get(pathname); break;
        //case 1: put(pathname); break;
        case 2: ls(pathname); break;
        //case 3: cd(pathname); break;
        case 4: pwd(); break;
        //case 5: mkdir(pathname);    break;
        //case 6: rmdir(pathname); break;
        //case 7: rm(pathname); break;
        }
      strcat(line, " ECHO");

      // send the echo line to client 
      //n = write(client_sock, line, MAX);

      //printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
      printf("server: ready for next request\n");
    }
 }
}


