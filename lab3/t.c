/*********** A Multitasking System ************/
#include <stdio.h>
#include "type.h"

PROC proc[NPROC], *running, *freeList, *readyQueue, *sleepList;

int ksleep(int event)
{
  printf("proc %d sleep on %d: ", running->pid, event);
  running->event = event;         // sleep event value
  running->status = SLEEP;        // status = SLEEP
  enqueue(&sleepList, running);   // enter into sleepList
  //printsleepList();
  tswitch();                      // switch process to give up CPU 
}

int kwakeup(int event)
{
  PROC *temp = 0;
  PROC *p;
  while(p = dequeue(&sleepList)){
    if (p->status == SLEEP && p->event == event){
      p->status = READY;
      enqueue(&readyQueue, p);
      printf("wakeup %d : ", p->pid);
      printList("readyQueue", readyQueue);
    }
    else{
      enqueue(&temp, p);
    }
  }
  sleepList = temp;
}

int do_switch()
{
   printf("proc %d switching task\n", running->pid);
   tswitch();
   printf("proc %d resuming\n", running->pid);
}

int do_kfork()
{
   int child = kfork();
   if (child < 0)
      printf("kfork failed\n");
   else{
      printf("proc %d kforked a child = %d\n", running->pid, child); 
      printList("readyQueue", readyQueue);
   }
   return child;
}

char *gasp[NPROC]={
  "Oh! I'm dying ....", 
  "Oh! You're killing me ....",
  "Oh! I'm a goner ....",
  "Oh! I'm mortally wounded ....",
};

int kexit(){
  int i;
  PROC *p, *cur = NULL;
  printf("proc %d in kexit()\n", running->pid);
  if (running->pid==1){
      printf("P1 never dies\n");
      return -1;
  }
  
  //(1). record pid as exitStatus;
  //(2). become a ZOMBIE;
  //(3). send children to P1; wakeup P1 if has sent any child to P1;
  //(4). kwakeup(parentPID);  // parent may be sleeping on its PID in wait()
  
  running->exitCode = running->pid;
  running->status = ZOMBIE;
  
  while(running->child != NULL){
	  cur = running->child;
	//cur = cur->sibling;
	
	
	PROC *temp = NULL;
	temp = cur;
	temp->ppid = 1;
	cur = NULL;
    p = cur;
  }
  printf("\nFree list = ");
  printList("Free List", freeList);
  kwakeup(p->ppid);
 
  
  tswitch();  // give up CPU 
}

int do_exit()
{
  if (running->pid==1){
    printf("P1 never dies\n");
    return -1;
  }
  kexit();    // journey of no return 
}

int kwait(int *status)
{
 PROC *temp;

  int i, zombies = 0, procs = NPROC;


	for(i = 0;i<NPROC;i++) {
	temp = &proc[i];
	if(temp->status==ZOMBIE) { //is a zombie
		*status = temp->exitCode;
		temp->status = FREE; //put proc into freelist
		temp->parent=0;
		enqueue(&freeList, temp);
		zombies++; 			//zombie exist, count up
		return temp->pid; 	//return zombie child p

	}

    }
    if(zombies==0) {
	return -1;
	ksleep(running);
	}
}

int do_wait()
{
  int pid, status;
  pid = kwait(&status);
  printf("proc %d waited for a ZOMBIE child %d status=%d\n", 
         running->pid, pid, status);
}

int body()
{
  int c, CR;
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printf("***************************************\n");
    printf("proc %d running: Parent = %d\n", running->pid, running->ppid);
 // ASSIGNMENT 3: add YOUR CODE to show child list
	showChild(running);
    printf("input a char [f|s|q|w] : ");
    c = getchar(); CR=getchar(); 
    switch(c){
      case 'f': do_kfork();     break;
      case 's': do_switch();    break;
      case 'q': do_exit();      break;
      case 'w': do_wait();      break;     // implement wait() operation
      default :                 break;  
    }
  }
}

void enter_child(PROC **queue, PROC *p) {
	PROC *newNode = (PROC*)malloc(sizeof(PROC));
	newNode->sibling = NULL;
	newNode->pid = p->pid;
	newNode->priority = p->priority;
	
	PROC *cur = *queue;
	
	if (*queue == NULL) {
		*queue = newNode;
	} else {
		if (newNode->priority > cur->priority) {
			newNode->sibling = *queue;
			*queue = newNode;
		} else {
			while (cur->sibling != NULL && cur->sibling->priority >= newNode->priority) {
				cur = cur->sibling;
			}
			if (cur->sibling == NULL) {
				cur->sibling = newNode;
			} else {
				newNode->sibling = cur->sibling;
				cur->sibling = newNode;
			}
		}
	}
}

int showChild (PROC *p) {
	printf("ChildList = ");
	while (p != NULL) {
		if (p->sibling == NULL) {
			printf("[%d %d]\n", p ->pid, p->priority);
		} else {
			printf("[%d %d] -> ", p->pid, p->priority);
		}
		p = p->sibling;
	}
}
     
/*******************************************************
  kfork() creates a child proc; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/
int kfork()
{
  PROC *p;
  int  i;
  /*** get a proc from freeList for child proc: ***/
  p = dequeue(&freeList);
  if (!p){
     printf("no more proc\n");
     return(-1);
  }

  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1;         // for ALL PROCs except P0
  p->ppid = running->pid;

  //                    -1   -2  -3  -4  -5  -6  -7  -8   -9
  // kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
  for (i=1; i<10; i++)
    p->kstack[SSIZE - i] = 0;

  p->kstack[SSIZE-1] = (int)body;
  p->saved_sp = &(p->kstack[SSIZE - 9]); 
  
/**************** ASSIGNMENT 3  ********************
  add YOUR code to implement the PROC tree as a BINARY tree
  enter_child(running, p);
****************************************************/
	enter_child(&running, p);

  enqueue(&readyQueue, p);
  return p->pid;
}

int init()
{
  int i;
  for (i = 0; i < NPROC; i++){
    proc[i].pid = i; 
    proc[i].status = FREE;
    proc[i].priority = 0;
    proc[i].next = (PROC *)&proc[(i+1)];
  }
  proc[NPROC-1].next = 0;
 
  freeList = &proc[0];        
  readyQueue = 0;
  sleepList = 0;

  // create P0 as the initial running process
  running = dequeue(&freeList);
  running->status = READY;
  running->priority = 0;  

  running->child = 0;  
  running->sibling = 0;  
  running->parent = running;

  printf("init complete: P0 running\n"); 
  printList("freeList", freeList);
}

/*************** main() ***************/
/*
main()
{
   printf("\nWelcome to 360 Multitasking System\n");
   init();
   kfork();  
   printf("P0: switch task\n");
     tswitch();
   printf("All dead. Happy ending\n");
}
*/

/*********** scheduler *************/
int scheduler()
{ 
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
      enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);  
}


