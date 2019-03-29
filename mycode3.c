/* mycode3.c: your portion of the kernel
 *
 *   	Below are procedures that are called by other parts of the kernel. 
 * 	Your ability to modify the kernel is via these procedures.  You may
 *  	modify the bodies of these procedures any way you desire (however,
 *  	you cannot change the interfaces).  
 */

#include "aux.h"
#include "sys.h"
#include "mycode3.h"

#define FALSE 0 // free s
#define TRUE 1

/* 	A sample semaphore table. You may change this any way you wish. 
 */

static struct {
	int valid;	// Is this a valid entry (was sem allocated)?
	int value;	// value of semaphore
    int blockList[MAXPROCS];  //the list of blocked processes on s
    int *head;  // head pointer
    int *tail;  // tail pointer
} semtab[MAXSEMS];


/* 	InitSem () is called when kernel starts up.  Initialize data
 *  	structures (such as semaphore table) and call any initialization
 *   	procedures here. 
 */

void InitSem ()
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {		// mark all sems free
        semtab[s].valid = FALSE;
	}
}

/* 	MySeminit (p, v) is called by the kernel whenever the system
 *  	call Seminit (v) is called.  The kernel passes the initial
 *  	value v, along with the process ID p of the process that called
 * 	Seminit.  MySeminit should allocate a semaphore (find free entry
 * 	in semtab and allocate), initialize that semaphore's value to v,
 *  	and then return the ID (i.e., index of allocated entry). 
 */

int MySeminit (int p, int v)
{
	int s;
    int i;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {
		if (semtab[s].valid == FALSE) {
			break;
		}
	}
	if (s == MAXSEMS) {
		DPrintf ("No free semaphores.\n");
		return (-1);
	}

	semtab[s].valid = TRUE;
	semtab[s].value = v;
    
    for (i = 0; i < MAXPROCS; i++) {
        semtab[s].blockList[i] = -1;
    }   // clear the list
    semtab[s].head = semtab[s].blockList;      // set the head pointer
    semtab[s].tail = semtab[s].blockList + 1;      // set the tail pointer

	return (s);
}

/*   	MyWait (p, s) is called by the kernel whenever the system call
 * 	Wait (s) is called. 
 */

void MyWait (p, s)
	int p;				// process
	int s;				// semaphore
{
	/* modify or add code any way you wish */

    int end, i;

	semtab[s].value--;
    
    if (semtab[s].value < 0) {
        
        end = semtab[s].tail - semtab[s].head;
        if (end >= MAXPROCS) {
            DPrintf("The block list is full.\n");
        }
        else {
            semtab[s].blockList[end - 1] = p;
            semtab[s].tail++;
        }
        
        Block (p);
        
        
    }
}

/*  	MySignal (p, s) is called by the kernel whenever the system call
 *  	Signal (s) is called.  
 */

void MySignal (p, s)
	int p;				// process
	int s;				// semaphore
{
	/* modify or add code any way you wish */
    int pid, i;

	semtab[s].value++;
    
    if (semtab[s].tail - semtab[s].head != 1) {
        pid = *(semtab[s].head);
        for (i = 1; i < semtab[s].tail - semtab[s].head; i++) {
            semtab[s].blockList[i - 1] = semtab[s].blockList[i];
        }
        semtab[s].tail--;
        
        Unblock (pid);
        

    }
    
}

