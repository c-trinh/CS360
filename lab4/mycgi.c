#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define MAX 10000
typedef struct {
    char *name;
    char *value;
} ENTRY;

ENTRY entry[MAX];

main(int argc, char *argv[]) 
{
  int i, m, r;
  char cwd[128];

  m = getinputs();    // get user inputs name=value into entry[ ]
  getcwd(cwd, 128);   // get CWD pathname

  printf("Content-type: text/html\n\n");
  printf("<p>pid=%d uid=%d cwd=%s\n", getpid(), getuid(), cwd);

  printf("<H1>Echo Your Inputs</H1>");
  printf("You submitted the following name/value pairs:<p>");
 
  for(i=0; i <= m; i++)
     printf("%s = %s<p>", entry[i].name, entry[i].value);
  printf("<p>");


  /*****************************************************************
   Write YOUR C code here to processs the command
         mkdir dirname
         rmdir dirname
         rm    filename
         cat   filename
         cp    file1 file2
         ls    [dirname] <== ls CWD if no dirname
  *****************************************************************/

  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#FF0000\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");
  
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~trinh/cgi-bin/mycgi.bin\">");

  //------ NOTE : CHANGE ACTION to YOUR login name ----------------------------
  //printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~YOURNAME/cgi-bin/mycgi.bin\">");
  
  printf("Enter command : <INPUT NAME=\"command\"> <P>");
  printf("Enter filename1: <INPUT NAME=\"filename1\"> <P>");
  printf("Enter filename2: <INPUT NAME=\"filename2\"> <P>");
  printf("Submit command: <INPUT TYPE=\"submit\" VALUE=\"Click to Submit\"><P>");
  printf("</form>");
  printf("------------------------------------------------<p>");

  printf("</body>");
  printf("</html>");
}

void cat(int argc, char *argv[])
{
  int fd;
  int i, n;
  char buf[1024];

  if (argc < 2) exit(1);

  fd = open(argv[1], O_RDONLY);

  if (fd < 0) exit(2);

  while (n = read(fd, buf, 1024)){
     for (i=0; i < n; i++)        
          putchar(buf[i]);
  }
}

 #define BLKSIZE 4096
 int fd, gd;
 char buf[4096];

 void cp(int argc, char *argv[])
 {
   int n, total=0;
   if (argc < 3) exit(1);

   fd = open(argv[1], O_RDONLY);
   if (fd < 0) exit(2);
   gd=open(argv[2],O_WRONLY|O_CREAT);
   if (gd < 0) exit(3);
 
   while (n=read(fd, buf, BLKSIZE))
   {
      write(gd, buf, n);
      total += n;
   }
   printf("total=%d\n",total);

   close(fd); close(gd);
 }
