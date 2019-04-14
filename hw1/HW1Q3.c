#include <stdio.h>

int *FP; // a global pointer

int main(int argc, char *argv[], char *env[])
{ int a,b,c;
  printf("enter main\n");
  a = 1; b = 2; c = 3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{ int d,e,f;
  printf("enter a\n");
  d = 4; e = 5; f = 6;
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{ int u,v,w,i;
  printf("enter B\n\n");
  u=7; v=8; w=9;
  asm("movl %ebp, FP");
  printf("FP Value (argc):\t%x\nFP Address (argv):\t%x\n", *FP, FP);
  printf("u Address (env):\t%x\n\n", &u);
  while(FP != &u)
    {
      FP--;
    }

  for (int i = 0; i < 100; i++)
    {
       printf("FP Value:\t\t%d\nFP Address:\t%x\n----------\n", *FP, FP);	
	FP++;	
    }
  
  printf("exit B\n");
}
