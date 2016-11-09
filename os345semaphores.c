// os345semaphores.c - OS Semaphores
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
#include "pq.h"

extern TCB tcb[];							// task control block
extern PQ* rQueue;

extern int superMode;						// system mode
extern Semaphore* semaphoreList;			// linked list of active semaphores

// **********************************************************************
// **********************************************************************
// signal semaphore
//
//	if task blocked by semaphore, then clear semaphore and wakeup task
//	else signal semaphore
//
void semSignal(Semaphore* s) {
	//printf("SemSignal(%s)\n", s->name);
	// assert there is a semaphore and it is a legal type
	assert("semSignal Error" && s && ((s->type == 0) || (s->type == 1)));

	// check semaphore type
	Tid tid = ready(s->pq, rQueue);		// pop next waiting task
	if (s->type == BINARY) {
		if (tid != -1) {
			s->state = 0;				// clear semaphore
			tcb[tid].event = 0;			// clear event pointer
			tcb[tid].state = S_READY;	// unblock task
		} else {
			s->state = 1;
		}
		if (!superMode)
			swapTask();
		return;
	} else {
		if (tid != -1) {
			tcb[tid].event = 0;			// clear event pointer
			tcb[tid].state = S_READY;	// unblock task
		} else {
			// nothing waiting, signal
		}
		s->state++;						// increment counting semaphore
		if (!superMode)
			swapTask();
		return;
	}
} // end semSignal

// **********************************************************************
// **********************************************************************
// wait on semaphore
//
//	if semaphore is signaled, return immediately
//	else block task
//
int semWait(Semaphore* s) {
	//printf("SemWait(%s)\n", s->name);
	assert("semWait Error" && s);							// assert semaphore
	assert("semWait Error" && ((s->type == 0) || (s->type == 1)));// assert legal type
	assert("semWait Error" && !superMode);					// assert user mode

	if (s->type == BINARY) {
		if (s->state == 0 || s->pq->size > 0) {
			tcb[getCurTask()].event = s;	// block task
			tcb[getCurTask()].state = S_BLOCKED;

			block(rQueue, s->pq, getCurTask());

			swapTask();						// reschedule the tasks
			return 1;
		} else {
			s->state = 0;					// reset state, and don't block
			return 0;
		}
	} else {
		if (s->state < 1 || s->pq->size > 0) {
			tcb[getCurTask()].event = s;	// block task
			tcb[getCurTask()].state = S_BLOCKED;

			block(rQueue, s->pq, getCurTask());

			s->state = 0;					// reset state
			swapTask();						// reschedule the tasks
			return 1;
		} else {
			s->state--;					// reset state, and don't block
			return 0;
		}
	}
} // end semWait

// **********************************************************************
// **********************************************************************
// try to wait on semaphore
//
//	if semaphore is signaled, return 1
//	else return 0
//
int semTryLock(Semaphore* s) {
	assert("semTryLock Error" && s);						// assert semaphore
	assert("semTryLock Error" && ((s->type == 0) || (s->type == 1)));// assert legal type
	assert("semTryLock Error" && !superMode);				// assert user mode

	// check semaphore type
	if (s->type == BINARY) {
		// binary semaphore
		// if state is zero, then block task

		if (s->state == 0) {
			return 0;
		}
		// state is non-zero (semaphore already signaled)
		s->state = 0;						// reset state, and don't block
		return 1;
	} else {
		return 1;
		//counting semaphore
	}
} // end semTryLock

// **********************************************************************
// **********************************************************************
// Create a new semaphore.
// Use heap memory (malloc) and link into semaphore list (Semaphores)
// 	name = semaphore name
//		type = binary (0), counting (1)
//		state = initial semaphore state
// Note: memory must be released when the OS exits.
//
Semaphore* createSemaphore(char* name, int type, int state) {
	//printf("createSemaphore(%s) -> t:%i\n", name, curTask->tid);
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert semaphore is binary or counting
	assert("createSemaphore Error" && ((type == 0) || (type == 1)));// assert type is validate

	// look for duplicate name
	while (sem) {
		if (!strcmp(sem->name, name)) {
			printf("Semaphore %s already defined\n", sem->name);
			// semaphore found - change to new state
			if (sem->type == type) {
				return sem;
			} else {
				return 0;
			}
		}
		// move to next semaphore
		semLink = (Semaphore**) &sem->semLink;
		sem = (Semaphore*) sem->semLink;
	}

	// allocate memory for new semaphore
	sem = (Semaphore*) malloc(sizeof(Semaphore));

	// set semaphore values
	sem->name = (char*) malloc(strlen(name) + 1);
	strcpy(sem->name, name);				// semaphore name
	sem->type = type;							// 0=binary, 1=counting
	sem->state = state;						// initial semaphore state
	sem->taskNum = getCurTask();					// set parent task #
	sem->pq = newPqueue(CAP_INCREASE, sem->name);		// init task queue

	// prepend to semaphore list
	sem->semLink = (struct semaphore*) semaphoreList;
	semaphoreList = sem;						// link into semaphore list
	return sem;									// return semaphore pointer
} // end createSemaphore

// **********************************************************************
// **********************************************************************
// Delete semaphore and free its resources
//
bool deleteSemaphore(Semaphore** semaphore) {
	//printf("deleteSemaphore(%s) -> t:%i\n", (*semaphore)->name, curTask->tid);
	Semaphore* sem = semaphoreList;
	Semaphore** semLink = &semaphoreList;

	// assert there is a semaphore
	assert("deleteSemaphore Error" && *semaphore);

	// look for semaphore
	while (sem) {
		if (sem == *semaphore) {
			// semaphore found, delete from list, release memory
			*semLink = (Semaphore*) sem->semLink;

			// free the name array before freeing semaphore
			printf("deleteSemaphore(%s)\n", sem->name);
			free(sem->name);
			free(sem->pq);
			// ?? What should you do if there are tasks in this
			//    semaphores blocked queue????

			free(sem);

			return TRUE;
		}
		// move to next semaphore
		semLink = (Semaphore**) &sem->semLink;
		sem = (Semaphore*) sem->semLink;
	}

	// could not delete
	return FALSE;
} // end deleteSemaphore
