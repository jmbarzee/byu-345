// os345.c - OS Kernel	09/12/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "os345.h"
#include "os345signals.h"
#include "os345config.h"
#include "os345lc3.h"
#include "os345fat.h"
#include "pqueue.h"

// **********************************************************************
//	local prototypes
//
void pollInterrupts(void);
static int scheduler(void);
static void swapTaskOut(Pqueue* q);
static void swapTaskIn(PqEntry*  newTask);
static int dispatcher(void);

//static void keyboard_isr(void);
//static void timer_isr(void);

int sysKillTask(int taskId);
static int initOS(void);

// **********************************************************************
// **********************************************************************
// global semaphores

Semaphore* semaphoreList;			// linked list of active semaphores

Semaphore* keyboard;				// keyboard semaphore
Semaphore* charReady;				// character has been entered
Semaphore* inBufferReady;			// input buffer ready semaphore

Semaphore* tics10sec;				// 10 second semaphore
Semaphore* tics1sec;				// 1 second semaphore
Semaphore* tics10thsec;				// 1/10 second semaphore

// **********************************************************************
// **********************************************************************
// global system variables

TCB tcb[MAX_TASKS];					// task control block
Semaphore* taskSems[MAX_TASKS];		// task semaphore
jmp_buf k_context;					// context of kernel stack
jmp_buf reset_context;				// context of kernel stack
volatile void* temp;				// temp pointer used in dispatcher

int scheduler_mode;					// scheduler mode
int superMode;						// system mode
PqEntry* curTask;						// current task #
Pqueue* rQueue;						// task ready queue
long swapCount;						// number of re-schedule cycles
char inChar;						// last entered character
int charFlag;						// 0 => buffered input
int inBufIndx;						// input pointer into input buffer
char inBuffer[INBUF_SIZE+1];		// character input buffer
//Message messages[NUM_MESSAGES];		// process message buffers

int pollClock;						// current clock()
int lastPollClock;					// last pollClock
bool diskMounted;					// disk has been mounted

time_t oldTime10;					// old 10sec time
time_t oldTime1;					// old 1sec time
clock_t myClkTime;
clock_t myOldClkTime;


// **********************************************************************
// **********************************************************************
// OS startup
//
// 1. Init OS
// 2. Define reset longjmp vector
// 3. Define global system semaphores
// 4. Create CLI task
// 5. Enter scheduling/idle loop
//
int main(int argc, char* argv[])
{
	// save context for restart (a system reset would return here...)
	int resetCode = setjmp(reset_context);
	superMode = TRUE;						// supervisor mode

	switch (resetCode)
	{
		case POWER_DOWN_QUIT:				// quit
			powerDown(0);
			printf("\nGoodbye!!");
			return 0;

		case POWER_DOWN_RESTART:			// restart
			powerDown(resetCode);
			printf("\nRestarting system...\n");
			break;

		case POWER_UP:						// startup
			break;

		default:
			printf("\nShutting down due to error %d", resetCode);
			powerDown(resetCode);
			return resetCode;
	}
	// output header message
	printf("%s", STARTUP_MSG);

	for (int i=0; i<argc; i++) {
		int j = 0;
		char flag;
		if (argv[i][j++] == '-') {
			while ((flag = argv[i][j]) != '\0') {
				j++;
				switch (flag) {
					case 'p': {
						printParser = 1;		// -p
						break;
					}
					case 'r': {
						printParserReads = 1;	// -r
						printParser = 1;		// -p	(inherited)
						break;
					}
					case 'i': {
						printInterrupts = 1;		// -i
						break;
					}
					case 'k': {
						printInterruptKeystrokes = 1; // -k
						printInterrupts = 1;		// -i	(inherited)
						break;
					}
					case 's': {
						printSignals = 1;		// -s
						break;
					}
					case 'f': {
						printfnNames = 1;		// -f
						break;
					}
					case 'm': {
						printMallocs = 1;		// -m
						break;
					}
					case 'e': {
						printErrors = 1;		// -e
						break;
					}
					default: {

					}
				}
			}
		}
	}
	printf("flags p%i, r%i, i%i, k%i, s%i, f%i, m%i, e%i\n",
			printParser,
			printParserReads,
			printInterrupts,
			printInterruptKeystrokes,
			printSignals,
			printfnNames,
			printMallocs,
			printErrors);


	// initalize OS
	if ( resetCode = initOS()) return resetCode;

	// create global/system semaphores here

	swapTaskIn(newPqEntry(0, MED_PRIORITY));	// list shell task as running

	charReady = createSemaphore("charReady", BINARY, 0);
	inBufferReady = createSemaphore("inBufferReady", BINARY, 0);
	keyboard = createSemaphore("keyboard", BINARY, 1);
	tics10sec = createSemaphore("tics10sec", COUNTING, 0);
	tics1sec = createSemaphore("tics1sec", COUNTING, 0);
	tics10thsec = createSemaphore("tics10thsec", BINARY, 0);

	// schedule CLI task
	createTask("myShell",			// task name
					P1_shellTask,	// task
					MED_PRIORITY,	// task priority
					argc,			// task arg count
					argv);			// task argument pointers

	// Scheduling loop
	// 1. Check for asynchronous events (character inputs, timers, etc.)
	// 2. Choose a ready task to schedule
	// 3. Dispatch task
	// 4. Loop (forever!)

	while(1)									// scheduling loop
	{
		//sleep(1);
		// check for character / timer interrupts
		pollInterrupts();

		// schedule highest priority ready task
		if (scheduler() < 0) continue;

		// dispatch curTask, quit OS if negative return
		if (dispatcher() < 0) break;

	}											// end of scheduling loop

	// exit os
	longjmp(reset_context, POWER_DOWN_QUIT);
	return 0;
} // end main



// **********************************************************************
// **********************************************************************
// scheduler
//
static int scheduler()
{
	if (curTask) {
		if (tcb[curTask->tid].event == 0) {
			swapTaskOut(rQueue);
		} else {
			// task is in semaphore queue
		}
	}

	PqEntry* popped = pop(rQueue);
	if (popped->tid == -1) {
		free(popped);
		return -1;
	} else {
		swapTaskIn(popped);
		if (tcb[curTask->tid].signal & mySIGSTOP) return -1;
		return curTask->tid;
	}
} // end scheduler

static void swapTaskOut(Pqueue* q) {
	//printf("swapTaskOut(%s) -> t: %i  p: %i\n", q->name, curTask->tid, curTask->priority);
	put(q, curTask);
}

static void swapTaskIn(PqEntry* newTask) {
	//printf("swapTaskIn(t: %i  p: %i)\n", newTask->tid, newTask->priority);
	curTask = newTask;
}



// **********************************************************************
// **********************************************************************
// dispatch curTask
//
static int dispatcher()
{
	int result;

	// schedule task
	switch(tcb[curTask->tid].state)
	{
		case S_NEW:
		{
			// new task
			printf("New Task[%d] %s\n", curTask->tid, tcb[curTask->tid].name);
			tcb[curTask->tid].state = S_RUNNING;	// set task to run state

			// save kernel context for task SWAP's
			if (setjmp(k_context))
			{
				superMode = TRUE;					// supervisor mode
				break;								// context switch to next task
			}

			// move to new task stack (leave room for return value/address)
			temp = (int*)tcb[curTask->tid].stack + (STACK_SIZE-8);
			SET_STACK(temp);
			superMode = FALSE;						// user mode

			// begin execution of new task, pass argc, argv
			result = (*tcb[curTask->tid].task)(tcb[curTask->tid].argc, tcb[curTask->tid].argv);

			// task has completed
			if (result) printf("Task[%d] returned %d\n", curTask->tid, result);
			else printf("Task[%d] returned %d\n", curTask->tid, result);
			tcb[curTask->tid].state = S_EXIT;			// set task to exit state

			// return to kernal mode
			longjmp(k_context, 1);					// return to kernel
		}

		case S_READY:
		{
			tcb[curTask->tid].state = S_RUNNING;			// set task to run
		}

		case S_RUNNING:
		{
			if (setjmp(k_context))
			{
				// SWAP executed in task
				superMode = TRUE;					// supervisor mode
				break;								// return from task
			}
			if (signals()) break;
			longjmp(tcb[curTask->tid].context, 3); 		// restore task context
		}

		case S_BLOCKED:
		{
			break;
		}

		case S_EXIT:
		{
			if (curTask->tid == 0) return -1;			// if CLI, then quit scheduler
			// release resources and kill task
			sysKillTask(curTask->tid);					// kill current task
			break;
		}

		default:
		{
			printf("Unknown Task[%d] State", curTask->tid);
			longjmp(reset_context, POWER_DOWN_ERROR);
		}
	}
	return 0;
} // end dispatcher



// **********************************************************************
// **********************************************************************
// Do a context switch to next task.

// 1. If scheduling task, return (setjmp returns non-zero value)
// 2. Else, save current task context (setjmp returns zero value)
// 3. Set current task state to READY
// 4. Enter kernel mode (longjmp to k_context)

void swapTask()
{
	assert("SWAP Error" && !superMode);		// assert user mode

	// increment swap cycle counter
	swapCount++;

	// either save current task context or schedule task (return)
	if (setjmp(tcb[curTask->tid].context))
	{
		superMode = FALSE;					// user mode
		return;
	}

	// context switch - move task state to ready
	if (tcb[curTask->tid].state == S_RUNNING) tcb[curTask->tid].state = S_READY;

	// move to kernel mode (reschedule)
	longjmp(k_context, 2);
} // end swapTask



// **********************************************************************
// **********************************************************************
// system utility functions
// **********************************************************************
// **********************************************************************

// **********************************************************************
// **********************************************************************
// initialize operating system
static int initOS()
{
	int i;

	// make any system adjustments (for unblocking keyboard inputs)
	INIT_OS

	// reset system variables
	rQueue = newPqueue(DEFAULT_CAP, "Ready Queue");
	curTask = 0;
	swapCount = 0;						// number of scheduler cycles
	scheduler_mode = 0;					// default scheduler
	inChar = 0;							// last entered character
	charFlag = 0;						// 0 => buffered input
	inBufIndx = 0;						// input pointer into input buffer
	semaphoreList = 0;					// linked list of active semaphores
	diskMounted = 0;					// disk has been mounted

	// capture current time
	lastPollClock = clock();			// last pollClock
	time(&oldTime1);
	time(&oldTime10);

	// init system tcb's
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;				// tcb
		taskSems[i] = NULL;				// task semaphore
	}

	// init tcb
	for (i=0; i<MAX_TASKS; i++)
	{
		tcb[i].name = NULL;
	}

	// initialize lc-3 memory
	initLC3Memory(LC3_MEM_FRAME, 0xF800>>6);

	// ?? initialize all execution queues

	return 0;
} // end initOS



// **********************************************************************
// **********************************************************************
// Causes the system to shut down. Use this for critical errors
void powerDown(int code)
{
	int i;
	printf("\nPowerDown Code %d", code);

	// release all system resources.
	printf("\nRecovering Task Resources...");

	// kill all tasks
	for (i = MAX_TASKS-1; i >= 0; i--)
		if(tcb[i].name) sysKillTask(i);

	// delete all semaphores
	while (semaphoreList)
		deleteSemaphore(&semaphoreList);

	// free ready queue
	emptyPqueue(rQueue);
	free(rQueue);

	// ?? deltaclock (project 3)

	RESTORE_OS
	return;
} // end powerDown

