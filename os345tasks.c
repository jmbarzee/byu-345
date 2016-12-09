// os345tasks.c - OS create/kill task	08/08/2013
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

#include "os345.h"
#include "os345signals.h"
#include "pq.h"
//#include "os345config.h"


extern TCB tcb[];							// task control block
extern PQ* rQueue;						// task ready queue

extern int superMode;						// system mode
extern Semaphore* semaphoreList;			// linked list of active semaphores
extern Semaphore* taskSems[MAX_TASKS];		// task semaphore



int createTimeTask(char* name,					// task name
					int (*task)(int, char**),	// task address
					int priority,				// task priority
					int argc,					// task argument count
					char* argv[],				// task argument pointers
					int sliceMult) {
	Tid tid = createTask(name, task, priority, argc, argv);
	tcb[tid].sliceMult = sliceMult;
	return tid;
}


// **********************************************************************
// **********************************************************************
// create task
int createTask(char* name,						// task name
					int (*task)(int, char**),	// task address
					int priority,				// task priority
					int argc,					// task argument count
					char* argv[])				// task argument pointers
{
	int tid;
	char** newArgv = malloc(sizeof(char*)*argc);
	for (int i = 0; i < argc; i++) {
		newArgv[i] = malloc(sizeof(char)*(strlen(argv[i])+1));
		strcpy(newArgv[i], argv[i]);
	}

	// find an open tcb entry slot
	for (tid = 0; tid < MAX_TASKS; tid++)
	{
		if (tcb[tid].name == 0)
		{
			char buf[8];

			// create task semaphore
			if (taskSems[tid]) deleteSemaphore(&taskSems[tid]);
			sprintf(buf, "task%d", tid);
			taskSems[tid] = createSemaphore(buf, 0, 0);
			taskSems[tid]->taskNum = 0;	// assign to shell

			// copy task name
			tcb[tid].name = (char*)malloc(strlen(name)+1);
			strcpy(tcb[tid].name, name);

			// set task address and other parameters
			tcb[tid].task = task;			// task address
			tcb[tid].state = S_NEW;			// NEW task state
			tcb[tid].priority = priority;	// task priority
			tcb[tid].parent = getCurTask();		// parent
			tcb[tid].argc = argc;			// argument count

			// ?? malloc new argv parameters
			tcb[tid].argv = newArgv;			// argument pointers

			tcb[tid].event = 0;				// suspend semaphore
			tcb[tid].RPT = 0;					// root page table (project 5)
			tcb[tid].cdir = CDIR;			// inherit parent cDir (project 6)
			tcb[tid].sliceMult = tcb[getCurTask()].sliceMult;
			tcb[tid].slices = 0;

			// define task signals
			createTaskSigHandlers(tid);

			// Each task must have its own stack and stack pointer.
			tcb[tid].stack = malloc(STACK_SIZE * sizeof(int));

			put(rQueue, tid);
			if (tid) {
				swapTask();				// do context switch (if not cli)
			}
			return tid;							// return tcb index (curTask)
		}
	}
	// tcb full!
	return -1;
} // end createTask

int taskPriority(Tid tid) {
	if (tid > -1)
		return tcb[tid].priority;
	return tid;
}

Tid taskParent(Tid tid) {
	if (tid > -1)
		return tcb[tid].parent;
	return tid;
}

char* taskName(Tid tid) {
	if (tid > -1)
		return tcb[tid].name;
	return 0;
}

int getSlices(Tid tid) {
	if (tid > -1)
		return tcb[tid].slices;
	return tid;
}

int dropSlice(Tid tid) {
	if (tid > -1)
		return --tcb[tid].slices;
	return tid;
}

void setSlices(Tid tid, int newSlices) {
	if (tid > -1)
		tcb[tid].slices = newSlices * tcb[tid].sliceMult;
}





// **********************************************************************
// **********************************************************************
// kill task
//
//	taskId == -1 => kill all non-shell tasks
//
static void exitTask(int tid);
int killTask(int taskId)
{
	if (taskId != 0)			// don't terminate shell
	{
		if (taskId < 0)			// kill all tasks
		{
			int tid;
			for (tid = 1; tid < MAX_TASKS; tid++)
			{
				if (tcb[tid].name) exitTask(tid);
			}
		}
		else
		{
			// terminate individual task
			if (!tcb[taskId].name) return 1;
			exitTask(taskId);	// kill individual task
		}
	}
	if (!superMode) SWAP;
	return 0;
} // end killTask

static void exitTask(Tid tid)
{
	assert("exitTaskError" && tcb[tid].name);

	Semaphore* sem = semaphoreList;
	while (sem) {
		if (pull(sem->pq, tid) != -1)
			put(rQueue, tid);
		sem = (Semaphore*) sem->semLink;
	}

	tcb[tid].state = S_EXIT;			// EXIT task state
	return;
} // end exitTask



// **********************************************************************
// system kill task
//
int sysKillTask(Tid tid)
{
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert that you are not pulling the rug out from under yourself!
	assert("sysKillTask Error" && tcb[tid].name && superMode);
	printf("\nKill Task %s", tcb[tid].name);

	// signal task terminated
	semSignal(taskSems[tid]);

	// look for any semaphores created by this task
	sem = *semLink;
	while(sem)
	{
		if(sem->taskNum == tid)
		{
			// semaphore found, delete from list, release memory
			deleteSemaphore(semLink);
		}
		else
		{
			// move to next semaphore
			semLink = (Semaphore**)&sem->semLink;
		}
		sem = *semLink;
	}

	for (int i=0; i<tcb[tid].argc; i++) free((tcb[tid].argv)[i]);
	free(tcb[tid].argv);

	Tid cid = -1;
	for (Tid id = 0; id < MAX_TASKS; id++) {
		if (taskName(id) != 0 && taskParent(id) == tid) {
			if (cid == -1) {
				cid = id;
				tcb[cid].parent = tcb[tid].parent;
			} else {
				tcb[id].parent = cid;
			}
		}
	}

	pull(rQueue, tid); // ?? delete task from system queues
	setCurTask(0);

	tcb[tid].name = 0;			// release tcb slot
	return 0;
} // end sysKillTask
