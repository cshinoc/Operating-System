/* mycode2.c: your portion of the kernel
*
*      Below are procedures that are called by other parts of the kernel.
*      Your ability to modify the kernel is via these procedures.  You may
*      modify the bodies of these procedures any way you wish (however,
*      you cannot change the interfaces).
*/

#include "aux.h"
#include "sys.h"
#include "mycode2.h"

#define TIMERINTERVAL 1    // in ticks (tick = 10 msec)
#define L 100000

static int count = 0;
static int allocCPU;
static int averageCPU;
static int averageStride;


//A sample process table. You may change this any way you wish.
static struct {
    int valid;              // is this entry valid: 1 = yes, 0 = no
    int pid;                // process ID (as provided by kernel)
    int index;              // record the index of the process
    int request;            // is the request percentage
    int requestFlag;       // request successful? 1 = yes, 0 = no
    int utilize;
    int stride;             // stride = L/request
    int pass;               // total pass
} proctab[MAXPROCS];

typedef struct {
    int data[MAXPROCS];
    int front;
    int tail;
    int size;
}Queue;

static Queue *q;

Queue* CreateQ() {
    Queue* qu = (Queue*)malloc(sizeof(Queue));
    qu->front = 0;
    qu->tail = -1;
    qu->size = 0;
    return qu;
}

void AddQ(Queue* q, int item) {
    q->tail++;
    q->tail %= MAXPROCS;
    q->size++;
    q->data[q->tail] = item;
}

void DelItem(Queue* q, int item) {
    int n = 0;
    int v;

    while (q -> data[n] != item) {
        n++;
    }
    q -> tail--;

    for (v = n; v < q -> size; v++) {
        q -> data[v] = q -> data[v + 1];
    }

    q -> size--;

}

/*      InitSched () is called when the kernel starts up.  First, set the
 *      scheduling policy (see sys.h). Make sure you follow the rules
 *      below on where and how to set it.  Next, initialize all your data
 *      structures (such as the process table).  Finally, set the timer
 *      to interrupt after a specified number of ticks.
 */

void InitSched ()
{
    int i;

    /* First, set the scheduling policy. You should only set it
     * from within this conditional statement.  While you are working
     * on this assignment, GetSchedPolicy () will return NOSCHEDPOLICY.
     * Thus, the condition will be true and you may set the scheduling
     * policy to whatever you choose (i.e., you may replace ARBITRARY).
     * After the assignment is over, during the testing phase, we will
     * have GetSchedPolicy () return the policy we wish to test (and
     * the policy WILL NOT CHANGE during the entirety of a test).  Thus
     * the condition will be false and SetSchedPolicy (p) will not be
     * called, thus leaving the policy to whatever we chose to test
     * (and so it is important that you NOT put any critical code in
     * the body of the conditional statement, as it will not execute when
     * we test your program).
     */
    if (GetSchedPolicy () == NOSCHEDPOLICY) {       // leave as is
        SetSchedPolicy (ROUNDROBIN);             // set policy here
    }

    allocCPU = 0;
    averageCPU = 0;
    averageStride = 0;

    /* Initialize all your data structures here */
    for (i = 0; i < MAXPROCS; i++) {
        proctab[i].valid = 0;
        proctab[i].index = 0;
        proctab[i].request = 0;
        proctab[i].requestFlag = 0;
        proctab[i].utilize = 0;
        proctab[i].stride = 0;
        proctab[i].pass = 0;
    }

    q = CreateQ();

    /* Set the timer last */
    SetTimer (TIMERINTERVAL);
}

/*      StartingProc (p) is called by the kernel when the process
 *      identified by PID p is starting. This allows you to record the
 *      arrival of a new process in the process table, and allocate
 *      any resources (if necessary).  Returns 1 if successful, 0 otherwise.
 */

int StartingProc (p)
        int p;                          // process that is starting
{
    int i;

    //DPrintf("Process %d starts...\n",p);

    for (i = 0; i < MAXPROCS; i++) {
        if (! proctab[i].valid) {
            count++;
            proctab[i].valid = 1;
            proctab[i].pid = p;
            proctab[i].index = count;

            if(GetSchedPolicy() == LIFO) { DoSched(); }

            if(GetSchedPolicy() == ROUNDROBIN) {
                AddQ(q, p);
            }


            return (1);
        }
    }

    DPrintf ("Error in StartingProc: no free table entries\n");
    return (0);
}


/*      EndingProc (p) is called by the kernel when the process
 *      identified by PID p is ending.  This allows you to update the
 *      process table accordingly, and deallocate any resources (if
 *      necessary).  Returns 1 if successful, 0 otherwise.
 */


int EndingProc (p)
        int p;                          // process that is ending
{
    int i;

    for (i = 0; i < MAXPROCS; i++) {
        if (proctab[i].valid && proctab[i].pid == p) {
            proctab[i].valid = 0;

            if(GetSchedPolicy() == ROUNDROBIN ) {
                DelItem(q, p);
            }

            if(GetSchedPolicy() == PROPORTIONAL ) {
                if (proctab[i].requestFlag == 1) {
                    allocCPU -= proctab[i].utilize;
                }

                //DPrintf("Process %d ends...CPU allocation: %d\n", p, allocCPU);
            }

            return (1);
        }
    }

    DPrintf ("Error in EndingProc: can't find process %d\n", p);
    return (0);
}


/*      SchedProc () is called by kernel when it needs a decision for
 *      which process to run next. It calls the kernel function
 *      GetSchedPolicy () which will return the current scheduling policy
 *      which was previously set via SetSchedPolicy (policy).  SchedProc ()
 *      should return a process PID, or 0 if there are no processes to run.
 */

int SchedProc ()
{
    int i,j;
    int pp;
    int nonRequest;
    int minPass = L;
    int tempCPU = 0;

    switch (GetSchedPolicy ()) {

        case ARBITRARY:

            for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid) {
                    return (proctab[i].pid);
                }
            }
            break;

        case FIFO:

            /* your code here */
            for (j = 1; j <= count; j++) {
                for (i = 0; i < MAXPROCS; i++) {
                    if (proctab[i].valid && proctab[i].index == j) {
                        return (proctab[i].pid);
                    }
                }

            }

            break;

        case LIFO:

            /* your code here */
            for (j = count; j > 0; j--) {
                for (i = 0; i < MAXPROCS; i++) {
                    if (proctab[i].valid && proctab[i].index == j) {
                        return (proctab[i].pid);
                    }
                }

            }

            break;

        case ROUNDROBIN:

//            index = q->front;
//            int n;
//            for (n = 0; n < q->size; n++) {
//                index++;
//                index %= MAXPROCS;
//                DPrintf("%d ", q->data[index]);
//            }


            /* your code here */

            for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid) {

                    q -> front %= q -> size;
                    pp = q -> data[q -> front];
                    q -> front++;

                    return (pp);

                }
            }


            break;

        case PROPORTIONAL:

            /* your code here */

            //DPrintf("Start scheduling...\n");
            nonRequest = 0;
            minPass = L;

            for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid && proctab[i].requestFlag == 0 && proctab[i].request != 0) {

                    if (allocCPU - proctab[i].utilize + (int)(proctab[i].request * 0.9) + 1 <= 100) {
                        allocCPU = allocCPU - proctab[i].utilize + (int)(proctab[i].request * 0.9) + 1;
                        proctab[i].request = (int)(proctab[i].request * 0.9) + 1;
                        proctab[i].requestFlag = 1;
                        proctab[i].utilize = proctab[i].request;
                    }
                }

                if (proctab[i].valid && proctab[i].request == 0 && proctab[i].requestFlag == 0) {
                    nonRequest++;
                    //DPrintf("Process %d does not request\n", proctab[i].pid);
                }

            }

            //DPrintf("There are %d processes do not request and the allocated cpu is %d\n", nonRequest, allocCPU);

            if (nonRequest != 0 && allocCPU < 100) {
                averageStride = (int)(L * nonRequest/(100 - allocCPU));
                averageCPU = (int) ((100 - allocCPU) / nonRequest);
                minPass = nonRequest * L;
            }

            for (i = 0; i < MAXPROCS; i++) {
                if (nonRequest != 0 && proctab[i].valid && proctab[i].request == 0 && proctab[i].requestFlag == 0) {
                    proctab[i].stride = averageStride;
                    proctab[i].utilize = averageCPU;
                    //DPrintf("Pid %d's stride is %d\n",proctab[i].pid,proctab[i].stride);
                }

                if (proctab[i].valid && proctab[i].requestFlag) {
                    proctab[i].stride = (int)(L / proctab[i].request);
                    //DPrintf("Pid %d's stride is %d\n",proctab[i].pid,proctab[i].stride);
                }
            }

            for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid && proctab[i].stride != 0) {

                    if (proctab[i].pass <= minPass) {
                        minPass = proctab[i].pass;
                    }
                    //DPrintf("process %d's pass = %d\n", proctab[i].pid,proctab[i].pass);
                }
            }

            //DPrintf("The minPass is %d\n",minPass);

            if (minPass >= L) {
                for (i = 0; i < MAXPROCS; i++) {
                    if (proctab[i].valid && proctab[i].stride != 0) {
                        proctab[i].pass = 0;
                    }
                }
                minPass = 0;
            }

            averageStride = 0;

            for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid && proctab[i].pass == minPass) {
                    proctab[i].pass += proctab[i].stride;
                    //DPrintf("Return pid: %d\n",proctab[i].pid);
                    return (proctab[i].pid);
                }
            }


            break;

    }

    return (0);
}

/*      HandleTimerIntr () is called by the kernel whenever a timer
 *      interrupt occurs.  Timer interrupts should occur on a fixed
 *      periodic basis.
 */

void HandleTimerIntr ()
{
    SetTimer (TIMERINTERVAL);

    switch (GetSchedPolicy ()) {    // is policy preemptive?

        case ROUNDROBIN:                // ROUNDROBIN is preemptive
            DoSched ();             // make scheduling decision
            break;

        case PROPORTIONAL:              // PROPORTIONAL is preemptive

            DoSched ();             // make scheduling decision
            break;

        default:                        // if non-preemptive, do nothing
            break;
    }
}

/*      MyRequestCPUrate (p, n) is called by the kernel whenever a process
 *      identified by PID p calls RequestCPUrate (n).  This is a request for
 *      n% of CPU time, i.e., requesting a CPU whose speed is effectively
 *      n% of the actual CPU speed. Roughly n out of every 100 quantums
 *      should be allocated to the calling process. n must be at least
 *      0 and must be less than or equal to 100.  MyRequestCPUrate (p, n)
 *      should return 0 if successful, i.e., if such a request can be
 *      satisfied, otherwise it should return -1, i.e., error (including if
 *      n < 0 or n > 100). If MyRequestCPUrate (p, n) fails, it should
 *      have no effect on scheduling of this or any other process, i.e., AS
 *      IF IT WERE NEVER CALLED.
 */

int MyRequestCPUrate (p, n)
        int p;                          // process whose rate to change
        int n;                          // percent of CPU time

{
    int flag = 1;
    int temp = 0;
    int i;
    /* your code here */
    //DPrintf("Process %d requests %d cpu\n",p,n);
    if (n < 0 || n > 100) {
        return (-1);
    }

    if (n == 0) { flag = 0; }

//    for (i = 0; i < MAXPROCS; i++) {
//        if (proctab[i].valid && proctab[i].requestFlag == 1) {
//            allocCPU += proctab[i].request; //record the allocated cpu for the remaining processes
//            DPrintf("The allocated cpu is %d\n", allocCPU);
//        }
//    }

    //DPrintf("The allocated cpu is %d\n", allocCPU);
    if ((int)(n * 0.9) + 1 + allocCPU > 100 && n != 0) {
        temp = n;
        n = 100 - allocCPU;
        allocCPU = 100;
        flag = 0;                       // request fails
    }
    else {
        allocCPU += n;                  //add new n;
        temp = n;
    }

//    DPrintf("The allocated cpu is %d after this request.\n", allocCPU);
    for (i = 0; i < MAXPROCS; i++) {
        if (proctab[i].valid && proctab[i].pid == p) {
            proctab[i].utilize = n;
            proctab[i].requestFlag = flag;
            proctab[i].request = temp;
            //DPrintf("=============\nPid %d: request = %d requestFlag = %d utilize = %d\n============\n",proctab[i].pid, proctab[i].request, proctab[i].requestFlag, proctab[i].utilize);
        }
    }


    return (0);
}


