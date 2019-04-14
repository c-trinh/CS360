// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#define MAX 256

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

int server_sock, r;
int SERVER_IP, SERVER_PORT; 


// clinet initialization code

int client_init(char *argv[])
{
  printf("======= clinet init ==========\n");

  printf("1 : get server info\n");
  hp = gethostbyname(argv[1]);
  if (hp==0){
     printf("unknown host %s\n", argv[1]);
     exit(1);
  }

  SERVER_IP   = *(long *)hp->h_addr;
  SERVER_PORT = atoi(argv[2]);

  printf("2 : create a TCP socket\n");
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock<0){
     printf("socket call failed\n");
     exit(2);
  }

  printf("3 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("4 : connecting to server ....\n");
  r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(1);
  }

  printf("5 : connected OK to \007\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s  IP=%s  PORT=%d\n", 
          hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
  printf("---------------------------------------------------------\n");

  printf("========= init done ==========\n");
}

int type(char line[]){
  char *cmd[] = { "get", "put", "ls", "cd", "pwd", "mkdir", "rmdir", "rm", "lcat", "lls", "lcd", "lpwd", "lmkdir", "lrmdir", "lrm", 0};
  char command[MAX], pathname[MAX];
  sscanf(line, "%s %s", command, pathname);
  printf("cmd = [%s]\n", command);
  int i = 0;
  bool found = false;
  while (cmd[i]) {
    if (strcmp(command, cmd[i]) == 0){
      found = true;
      break;
    }
    i++;
  }
  if (found == true){
    if (i <= 7){
      return 1;
    }
    else if (i > 7 && i <= 14){
      return 2;
    }
  }
  else{
    printf("invalid command %s\n", command);
    return 0;
  }
}
main(int argc, char *argv[ ])
{
  int n;
  char line[MAX], ans[MAX];

  if (argc < 3){
     printf("Usage : client ServerName SeverPort\n");
     exit(1);
  }

  client_init(argv);
  // sock <---> server
  printf("********  processing loop  *********\n");
  while (1){
    printf("\n********************** menu *********************\n");
    printf("* get  put  ls   cd   pwd   mkdir   rmdir   rm  *\n");
    printf("* lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *\n");
    printf("*************************************************\n");
    printf("input a line : ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
    putchar('\n');
    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0)                  // exit if NULL line
       exit(0);


    
    int runner = type(line);
    printf("runner: %d\n", runner);

    if (runner == 2){ //LOCAL
      char cmd[MAX], pathname[MAX];
      
      DIR *dirp;
      struct dirent *ent;


      //strcpy(cmd, &line[1]);
      sscanf(&line[1], "%s %s", cmd, pathname); // similar to scanf() but from line[ ]
      //printf("cmd: %s\npathname: %s\n", cmd, pathname);

      
      if(!strcmp(cmd, "mkdir"))
      {
        
        int success = mkdir(pathname, 0777);
        if (success == 0){
          printf("lmkdir %s OK", pathname);
        }
        else {
          printf("lmkdir %s FAILED", pathname);
        }
      }
      else if(!strcmp(cmd, "rmdir"))
      {
        int success = rmdir(pathname);
        if (success == 0){
          printf("lrmdir %s OK", pathname);
        }
        else {
          printf("lrmdir %s FAILED", pathname);
        }
      }
      else if(!strcmp(cmd, "rm"))
      {
        
        int success = unlink(pathname);
        if (success == 0){
          printf("lrm %s OK", pathname);
        }
        else {
          printf("lrm %s FAILED", pathname);
        }
      }
      else if(!strcmp(cmd, "cat"))
      {
        printf("local cat\n");
        if(pathname != NULL)
        {
          if (pathname[0] != '/'){ //relative
            if (!getcwd(pathname, MAX)){printf("lcat %s failed\n", pathname); continue;}
          }
          FILE *fp = fopen(pathname, "r"); //file must exist
      
          if(fp)
          {
            char c;
            while((c=fgetc(fp)) != EOF)
            {
              putchar(c);
            }
          }
          else
          {
            printf("First entry File does not exist.");
          }
          fclose(fp);
        }
      }
      else if(!strcmp(cmd, "ls"))
      {	
        if (pathname[0] != '/'){ //relative
          if (getcwd(pathname, MAX) == NULL){
            printf("lls %s failed\n", pathname); 
            continue;
          }
        }
        printf("PATHNAME: %s\n", pathname);
        if ((dirp = opendir(pathname))!= NULL){
          while((ent = readdir(dirp)) != NULL)
          {
            printf("%s\n", ent->d_name);
          }
        }
        else {
          printf("lls %s FAILED\n", pathname);
        }

        
      }
      
      else if (!strcmp(cmd, "cd")) {//cd
        char mycwd[MAX] = {'\0'};
        int success = 2;
        if (pathname[0] != '/'){ //relative
          
          getcwd(mycwd, MAX);
          strcat(mycwd, "/");
          strcat(mycwd, pathname);
          success = chdir(mycwd);
        }
        else {
          success = chdir(pathname);
        }
        if (success == 0){
          printf("lcd %s OK", pathname);
        }
        else {
          printf("lcd %s FAILED", pathname);
        }

      }
      else if (!strcmp(cmd, "pwd")){
        char mycwd[MAX];
        getcwd(mycwd, MAX);
        printf("pwd=%s\n", mycwd);
        
      }
      bzero(pathname, MAX);
      bzero(cmd, MAX);
    }
    else if (runner ==1){
      // Send ENTIRE line to server
      n = write(server_sock, line, MAX);
      printf("client: wrote n=%d bytes; line=(%s)\n", n, line);
      
      
      // Read a line from sock and show it
      n = read(server_sock, ans, MAX);
      printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);
      
    }
  }
}



