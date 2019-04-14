#include "type.h"
#include <stdio.h>

PROC* root = NULL;
PROC proc[NPROC], *readyQueue, *freeList;

/*int main()
{
	int i;
	PROC *p;
	readyQueue = 0;

	for (i = 0; i < NPROC; i++) {
		p = &proc[i];
		p->pid = i;
		p->priority = rand() % 10;
		printf("pid=%d priority=%d\n", p->pid, p->priority);
		enqueue(&readyQueue, p);
		printList("readyQ", readyQueue);
	}
}*/

int enqueue(PROC **queue, PROC *p)
{
	PROC *head = root;
	PROC *temp;
	PROC *rot;
	if (root == NULL) {
		root = p;
		return 0;
	}
	if (p->priority < root->priority) {
		PROC *head = root;
		while ((head->next != NULL) && (head->next->priority > p->priority))
				head = head->next;
		rot = head->next;
		head->next = p;
		p->next = rot;
		return 0;
	}
	temp = root;
	root = p;
	p->next = temp;
	return 1;
}

PROC *dequeue(PROC **queue)
{
	PROC *temp;
	if (root != NULL)
	{
		temp = root;
		root = root->next;
		temp->next = NULL;
		return temp;
	}
}

int printList(char *name, PROC *p)
{
	PROC *temp = root;
	printf("%s = ", name);
	while (temp != NULL)
	{
		printf("[%d  %d] ->  ", temp->pid, temp->priority);
		temp = temp->next;
	}
	printf("NULL\n");
}
