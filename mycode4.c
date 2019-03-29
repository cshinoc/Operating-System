/*   	Umix thread package
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mycode4.h"
#include <stdlib.h>
#include "string.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;			// current context
    jmp_buf ini;
    void (*ff)();
    int pp;
    
} thread[MAXTHREADS];

#define STACKSIZE    65536        // maximum size of thread stack

static int last_assig;
static int curr_thread;
static int track_botton;
static int thread_list[MAXTHREADS];
static int run_thread;
static int call_flag;
static int prev_thread;
static int exit_flag;
static int total_thread;

/* 	MyInitThreads () initializes the thread package. Must be the first
 *  	function called by any user program that uses the thread package.  
 */

void MyInitThreads ()
{
    char stack[STACKSIZE];
    int i;
    

	if (MyInitThreadsCalled) {		// run only once
		Printf ("MyInitThreads: should be called only once\n");
		Exit ();
	}
    
    thread[0].valid = 1;            // initialize thread 0
    
    MyInitThreadsCalled = 1;
    
    last_assig = 0;    // initialize the pointer to position 1
    
    track_botton = MAXTHREADS;
    
    curr_thread = 0;
    
    run_thread = 1;
    
    call_flag = 0;
    
    prev_thread = -1;
    
    exit_flag = 0;
    
    total_thread = 1;
    
	for (i = 0; i < MAXTHREADS; i++) {	// initialize thread table
		thread[i].valid = 0;
        
        thread_list[i] = -1;
    }
    
    int rec;

    if ((rec = setjmp (thread[0].ini)) != 0) {
        curr_thread = rec - 1;
        (*thread[curr_thread].ff)(thread[curr_thread].pp);
        MyExitThread();
    }
    
    
    for (i = 1; i < MAXTHREADS; i++) {
        int a=1;
        char stack[i*STACKSIZE];
        
        if (((int) &stack[i*STACKSIZE-1]) - ((int) &stack[0]) + 1 != i*STACKSIZE) {
            Printf ("Stack space reservation failed\n");
            Exit ();
        }

        int b=1;
        DPrintf("%d\n",(int) &b - ((int) &a));
        
        if ((rec = setjmp (thread[i].ini)) != 0) {
            curr_thread = rec - 1;
            run_thread++;
            
            (*thread[curr_thread].ff)(thread[curr_thread].pp);
            MyExitThread();
        }
    }
    
    thread[0].valid = 1;
    
    memcpy(thread[0].env,thread[0].ini,sizeof(jmp_buf));
                 
    thread_list[0] = 0;
    
}

/*  	MyCreateThread (f, p) creates a new thread to execute
 * 	f (p), where f is a function with no return value and
 * 	p is an integer parameter.  The new thread does not begin
 *  	executing until another thread yields to it. 
 */

int MyCreateThread (f, p)
	void (*f)();			// function to be executed
	int p;				// integer parameter
{
	if (! MyInitThreadsCalled) {
		Printf ("MyCreateThread: Must call MyInitThreads first\n");
		Exit ();
	}

    // no unvalid thread return -1
    total_thread++;
    
    if (total_thread > 10) {
        total_thread = 10;
    }
    
    int count = 0;
    
    for (int i = 0; i < MAXTHREADS; i++) {
        
        if (thread[i].valid == 1) { count++; }
        
    }
    if (count == MAXTHREADS) {
        
        return -1;
        
    }
    
    int j = last_assig + 1;

    while (thread[j % MAXTHREADS].valid == 1) {
        j++;
    }

    int k = j % MAXTHREADS;
    
    last_assig = k;
    
	thread[k].valid = 1;	// mark the entry for the new thread valid
    
    memcpy(thread[k].env,thread[k].ini,sizeof(jmp_buf));
    
    thread[k].ff = f;
    
    thread[k].pp = p;
    
    track_botton = (track_botton + 1) % MAXTHREADS;
    
    thread_list[track_botton] = k;


	return (k);		// done, return new thread ID
}

/*   	MyYieldThread (t) causes the running thread, call it T, to yield to
 * 	thread t. Returns the ID of the thread that yielded to the calling
 *  	thread T, or -1 if t is an invalid ID.  Example: given two threads
 *  	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 * 	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 * 	will resume by returning from its call to MyYieldThread (2), which
 *  	will return the value 2.
 */

int MyYieldThread (t)
	int t;				// thread being yielded to
{
    
    
	if (! MyInitThreadsCalled) {
		Printf ("MyYieldThread: Must call MyInitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("MyYieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("MyYieldThread: Thread %d does not exist\n", t);
		return (-1);
	}
    
    int calling_thread = MyGetThread();
    
    prev_thread = calling_thread;
    
    
    // Yield to itself
    if (t == calling_thread) { return calling_thread; }
    

    
    // Yield to other        thread
    int rec;
    if ((rec = setjmp (thread[calling_thread].env)) == 0) {

        int l1 = 0, l2 = 0;
        
        while (l1 < MAXTHREADS){
            if (thread_list[l1] == t) { break; }
            else { l1++; }
        }
        
        if (l1 == MAXTHREADS - 1) {
            thread_list[l1] = -1;
        } else {
            for (int i = l1; i < MAXTHREADS - 1; i++) {
                thread_list[i] = thread_list[i+1];
            }
            thread_list[track_botton] = -1;
        }
        
        while (l2 < MAXTHREADS){
            if (thread_list[l2] == calling_thread) { break; }
            else { l2++; }
        }
        
        if (l2 == MAXTHREADS && exit_flag == 0){
            thread_list[track_botton + 1] = calling_thread;
        }else if (l2 == MAXTHREADS - 1) {
            thread_list[l2] = -1;
        } else {
            for (int i = l2; i < MAXTHREADS - 1; i++) {
                thread_list[i] = thread_list[i+1];
            }
        }
        
        for (int z = 0; z < MAXTHREADS; z++) {
            
            if (exit_flag == 1) {
                exit_flag = 0;
                track_botton--;
                prev_thread = -1;
                break;
            }
            if (thread_list[z] == -1 && thread[calling_thread].valid == 1) {
                thread_list[z] = calling_thread;
                track_botton = z;
                break;
            }
        }
        
        if (thread_list[track_botton] == -1) { track_botton --; }

        curr_thread = t;
        
        
        if (call_flag == 1) {
            call_flag = 0;
            prev_thread = -1;
        }
        
        longjmp (thread[t].env, t + 1); //in case of 0
    }
    
    curr_thread = rec - 1;
    
    return prev_thread;
}

/*   	MyGetThread () returns ID of currently running thread. 
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyGetThread: Must call MyInitThreads first\n");
		Exit ();
	}
    
    return curr_thread;

}

/* 	MySchedThread () causes the running thread to simply give up the
 *  	CPU and allow another thread to be scheduled.  Selecting which
 *  	thread to run is determined here.  Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads). 
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MySchedThread: Must call MyInitThreads first\n");
		Exit ();
	}
    

    if(total_thread == 0) {
        Exit();
    }
    
    int l = 0;
    while (l < MAXTHREADS){
        if (thread_list[l] == curr_thread) { break; }
        else { l++; }
    }
    
    if (thread[curr_thread].valid == 1) {
        if (l == MAXTHREADS) {
            thread_list[track_botton + 1] = curr_thread;
        } else if (l == MAXTHREADS - 1) {
            thread_list[l] = curr_thread;
        } else {
            for (int i = l; i < MAXTHREADS - 1; i++) {
                thread_list[i] = thread_list[i+1];
            }
            thread_list[track_botton] = curr_thread;
        }
    }
    
    
    call_flag = 1;
    MyYieldThread(thread_list[0]);
    
}

/* 	MyExitThread () causes the currently running thread to exit. 
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyExitThread: Must call MyInitThreads first\n");
		Exit ();
	}
    run_thread--;
    total_thread--;
    thread[curr_thread].valid = 0;
    
    if (total_thread == 0) {
        Exit();
    }
    

    int l = 0;
    while (l < MAXTHREADS){
        if (thread_list[l] == curr_thread) { break; }
        else { l++; }
    }
    
    if (l == MAXTHREADS) {
        exit_flag = 1;
        MyYieldThread(thread_list[0]);
    }
    else {
        if (l == MAXTHREADS - 1) {
            thread_list[l] = -1;

        } else {
            for (int i = l; i < MAXTHREADS - 1; i++) {
                thread_list[i] = thread_list[i+1];
            }
            thread_list[track_botton] = -1;
        }
         track_botton--;
        
        MySchedThread();
    }
}
