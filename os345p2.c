// os345p2.c - 5 state scheduling	08/08/2013
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
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
#include <assert.h>
#include <time.h>

#include "os345.h"
#include "os345signals.h"
#include "pq.h"

// ***********************************************************************
// project 2 variables
static Semaphore* s1Sem;					// task 1 semaphore
static Semaphore* s2Sem;					// task 2 semaphore

extern Semaphore* tics10sec;				// 10 second semaphore

extern TCB tcb[];						// task control block
extern PQ* rQueue;						// task ready queue

extern Semaphore* semaphoreList;			// linked list of active semaphores
extern jmp_buf reset_context;				// context of kernel stack

// ***********************************************************************
// project 2 functions and tasks

int signalTask(int, char**);
int ImAliveTask(int, char**);
int wait10SecTask(int, char**);

void printTask(int tid);

// ***********************************************************************
// ***********************************************************************
// project2 command
int P2_project2(int argc, char* argv[]) {
	static char* s1Argv[] = { "signal1", "s1Sem" };
	static char* s2Argv[] = { "signal2", "s2Sem" };
	static char* aliveArgv[] = { "I'm Alive", "3" };
	static char* wait10Argv[] = { "Wait10Seconds" };

	printf("Starting Project 2\n");
	SWAP
	;

	// start tasks looking for sTask semaphores
	createTask("signal1",				// task name
			signalTask,				// task
			VERY_HIGH_PRIORITY,		// task priority
			2,						// task argc
			s1Argv);				// task argument pointers

	createTask("signal2",				// task name
			signalTask,				// task
			VERY_HIGH_PRIORITY,		// task priority
			2,						// task argc
			s2Argv);				// task argument pointers

	createTask("I'm Alive",				// task name
			ImAliveTask,			// task
			LOW_PRIORITY,			// task priority
			2,						// task argc
			aliveArgv);				// task argument pointers

	createTask("I'm Alive",				// task name
			ImAliveTask,			// task
			LOW_PRIORITY,			// task priority
			2,						// task argc
			aliveArgv);				// task argument pointers*/
	for (int i = 0; i < 8; i++) {
		createTask("Wait10Seconds",			// task name
				wait10SecTask,			// task
				VERY_HIGH_PRIORITY,		// task priority
				1,						// task argc
				aliveArgv);				// task argument pointers
	}
	return 0;
} // end P2_project2

// ***********************************************************************
// ***********************************************************************
// list tasks command
int P2_listTasks(int argc, char* argv[]) {
	Semaphore* sem = semaphoreList;
	printf("%12s %s %s  %s\n", "Name", "Tid", "Pri", "State");
	if (rQueue > 0) {
		printf("Queue: %s\n", rQueue->name);
		for (int i = rQueue->size - 1; i > -1; i--) {
			printTask(rQueue->content[i]);
		}
	}
	sem = (Semaphore*) sem->semLink;
	while (sem) {
		PQ* q = sem->pq;
		if (q->size > 0) {
			printf("Queue: %s\n", sem->name);
			for (int i = q->size - 1; i > -1; i--) {
				printTask(q->content[i]);
			}
		}
		sem = (Semaphore*) sem->semLink;
		//SWAP
	}
	return 0;
} // end P2_listTasks

void printTask(Tid tid) {
	printf("%12s %3i %3i  ", tcb[tid].name, tid, taskPriority(tid));
	if (tcb[tid].signal & mySIGSTOP)
		printf("Paused");
	else if (tcb[tid].state == S_NEW)
		printf("New");
	else if (tcb[tid].state == S_READY)
		printf("Ready");
	else if (tcb[tid].state == S_RUNNING)
		printf("Running");
	else if (tcb[tid].state == S_BLOCKED)
		printf("Blocked    %s", tcb[tid].event->name);
	else if (tcb[tid].state == S_EXIT)
		printf("Exiting");
	printf("\n");
}

// ***********************************************************************
// ***********************************************************************
// list semaphores command
//
int match(char* mask, char* name) {
	int i, j;

	// look thru name
	i = j = 0;
	if (!mask[0])
		return 1;
	while (mask[i] && name[j]) {
		if (mask[i] == '*')
			return 1;
		else if ((mask[i] != toupper(name[j])) && (mask[i] != tolower(name[j])))
			return 0;
		i++;
		j++;
	}
	if (mask[i] == name[j])
		return 1;
	return 0;
} // end match

int P2_listSems(int argc, char* argv[])				// listSemaphores
{
	Semaphore* sem = semaphoreList;
	while (sem) {
		if ((argc == 1) || match(argv[1], sem->name)) {
			printf("%30s  %c  %d  %s\n", sem->name, (sem->type ? 'C' : 'B'),
					sem->state, tcb[sem->taskNum].name);
		}
		sem = (Semaphore*) sem->semLink;
	}
	return 0;
} // end P2_listSems

// ***********************************************************************
// ***********************************************************************
// reset system
int P2_reset(int argc, char* argv[])						// reset
{
	longjmp(reset_context, POWER_DOWN_RESTART);
	// not necessary as longjmp doesn't return
	return 0;

} // end P2_reset

// ***********************************************************************
// ***********************************************************************
// kill task

int P2_killTask(int argc, char* argv[])			// kill task
{
	int taskId = INTEGER(argv[1]);				// convert argument 1

	if (taskId > 0)
		printf("Kill Task %d\n", taskId);
	else
		printf("\nKill All Tasks\n");

	// kill task
	if (killTask(taskId))
		printf("\nkillTask Error!");

	return 0;
} // end P2_killTask

// ***********************************************************************
int P2_signal1(int argc, char* argv[])		// signal1
{
	semSignal(s1Sem);
	return 0;
} // end signal

int P2_signal2(int argc, char* argv[])		// signal2
{
	semSignal(s2Sem);
	return 0;
} // end signal

// ***********************************************************************
// ***********************************************************************
// signal task
//
#define COUNT_MAX	5
//
int signalTask(int argc, char* argv[]) {
	int count = 0;					// task variable

	// create a semaphore
	Semaphore** mySem = (!strcmp(argv[1], "s1Sem")) ? &s1Sem : &s2Sem;
	*mySem = createSemaphore(argv[1], 0, 0);

	// loop waiting for semaphore to be signaled
	while (count < COUNT_MAX) {
		semWait(*mySem);			// wait for signal
		printf("%s  Task[%d], count=%d\n", tcb[getCurTask()].name, getCurTask(),
				++count);
	}
	return 0;						// terminate task
} // end signalTask

// ***********************************************************************
// ***********************************************************************
// I'm alive task
int ImAliveTask(int argc, char* argv[]) {
	int i;							// local task variable
	while (1) {
		printf("(%d) I'm Alive!\n", getCurTask());
		for (i = 0; i < 100000; i++)
			swapTask();
	}
	return 0;						// terminate task
} // end ImAliveTask

// ***********************************************************************
// ***********************************************************************
// 10 sec wait task
//
int wait10SecTask(int argc, char* argv[]) {
	int count = 0;					// task variable

	// loop waiting for semaphore to be signaled
	while (1) {
		semWait(tics10sec);		// wait for signal
		printf("%s  Task[%d], count=%d\n", tcb[getCurTask()].name, getCurTask(),
				++count);
	}
	return 0;						// terminate task
} // end signalTask

// **********************************************************************
// **********************************************************************
// read current time
//
char* myTime(char* svtime) {
	time_t cTime;						// current time

	time(&cTime);						// read current time
	strcpy(svtime, asctime(localtime(&cTime)));
	svtime[strlen(svtime) - 1] = 0;		// eliminate nl at end
	return svtime;
} // end myTime
