#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX 10000
typedef struct {
    char *name;
    char *value;
} ENTRY;

ENTRY entry[MAX];

int main(int argc, char *argv[]) 
{
  int i, m, c;
  FILE* output_file = NULL;
  FILE* input_file = NULL;
  DIR* dp = NULL;
  struct dirent* ep = NULL;
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
 
  if(strcmp(entry[0].value, "mkdir") == 0)
  {
    mkdir(entry[1].value, umask(S_IWGRP));
    printf("<H1>%s was made in %s</H1>", entry[1].value, cwd);
   
  }
  else if(strcmp(entry[0].value, "rmdir") == 0)
  {
	printf("<H1>Goodbye %s</H1>", entry[1].value);
    rmdir(entry[1].value); 
  }
  else if(strcmp(entry[0].value, "rm") == 0)
  {
    unlink(entry[1].value);
  }
  else if(strcmp(entry[0].value, "cat") == 0)
  {
	
	printf("<p>");
    input_file = fopen(entry[1].value, "r");
    if(input_file)
    {
      while((c = getc(input_file)) != EOF)
      {
		if(c == '<')
		{
			printf("&#60;");
			
		}
		else if(c=='>')
		{
			putchar('&');
			putchar('g');
			putchar('t');
			putchar(';');
		}
		else if(c=='\n')
		{
			printf("<br />");
		}
		else
		{  
        putchar(c);
		}
      }
      printf("\n");
      fclose(input_file);
    }
    
    printf("</p>");
  }
  else if(strcmp(entry[0].value, "cp") == 0)
  {

    input_file = fopen(entry[1].value, "r");
    output_file = fopen(entry[2].value, "w");
    if(input_file)
    {
      while((c = getc(input_file)) != EOF)
      {
        fprintf(output_file, "%c", c);
      }
      fclose(output_file);
      fclose(input_file);
    }
    
  }
  else if(strcmp(entry[0].value, "ls") == 0)
  {
	  printf("<p>");
    if(strcmp(entry[1].value,""))
    {
      dp = opendir(entry[1].value);
    }
    else
    {
      dp = opendir("./");	
    }
    if(dp != NULL)
    {
		int j = 0;
		printf("<H1>Items in the Index</H1>");
      while (ep = readdir(dp))
      {
		printf("<p>item[%d]: ", j+1);
		
        puts(ep->d_name);
        printf("</p>");
        j++;
      }
      closedir(dp);
    }
    else
    {
      printf("Couldn't open the directory\n");
    }
    printf("</p>");
  }
  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#afeeee\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");
  
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~heath/cgi-bin/mycgi\">");

  //------ NOTE : CHANGE ACTION to YOUR login name ----------------------------
  //printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~YOURNAME/cgi-bin/mycgi\">");
  
  printf("Enter command : <INPUT NAME=\"command\"> <P>");
  printf("Enter filename1: <INPUT NAME=\"filename1\"> <P>");
  printf("Enter filename2: <INPUT NAME=\"filename2\"> <P>");
  printf("Submit command: <INPUT TYPE=\"submit\" VALUE=\"Click to Submit\"><P>");
  printf("</form>");
  printf("------------------------------------------------<p>");

  printf("</body>");
  printf("</html>");
}
