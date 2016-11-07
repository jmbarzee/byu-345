// os345signal.c - signals
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
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

#include "os345.h"
#include "os345signals.h"
#include "pqueue.h"

extern TCB tcb[];							// task control block
extern PqEntry* curTask;							// current task #

// ***********************************************************************
// ***********************************************************************
//	Call all pending task signal handlers
//
//	return 1 if task is NOT to be scheduled.
//
int signals(void) {
	//debugPrint('s', 'f', "signals()\n");
	int ret = 0;
	if (tcb[curTask->tid].signal) {
		debugPrint('s', ' ', "check task: %i signal: %04x\n", curTask->tid,
				tcb[curTask->tid].signal);
		if (tcb[curTask->tid].signal & mySIGCONT) {
			tcb[curTask->tid].signal &= ~mySIGCONT;
			(*tcb[curTask->tid].sigContHandler)();
		}
		if (tcb[curTask->tid].signal & mySIGINT) {
			tcb[curTask->tid].signal &= ~mySIGINT;
			(*tcb[curTask->tid].sigIntHandler)();
		}
		if (tcb[curTask->tid].signal & mySIGKILL) {
			tcb[curTask->tid].signal &= ~mySIGKILL;
			(*tcb[curTask->tid].sigKillHandler)();
		}
		if (tcb[curTask->tid].signal & mySIGTERM) {
			tcb[curTask->tid].signal &= ~mySIGTERM;
			(*tcb[curTask->tid].sigTermHandler)();
			ret = 1;
		}
		if (tcb[curTask->tid].signal & mySIGTSTP) {
			tcb[curTask->tid].signal &= ~mySIGTSTP;
			(*tcb[curTask->tid].sigTstpHandler)();
			ret = 1;
		}
	}
	return ret;
}

// **********************************************************************
// **********************************************************************
//	Register task signal handlers
//
int sigAction(void (*sigHandler)(void), int sig) {

	debugPrint('s', 'f', "sigAction()\n");
	switch (sig) {
	case mySIGCONT: {
		tcb[curTask->tid].sigContHandler = sigHandler;		// mySIGCONT handler
		return 0;
	}
	case mySIGINT: {
		tcb[curTask->tid].sigIntHandler = sigHandler;		// mySIGINT handler
		return 0;
	}
	case mySIGKILL: {
		tcb[curTask->tid].sigKillHandler = sigHandler;		// mySIGKILL handler
		return 0;
	}
	case mySIGTERM: {
		tcb[curTask->tid].sigTermHandler = sigHandler;		// mySIGTERM handler
		return 0;
	}
	case mySIGTSTP: {
		tcb[curTask->tid].sigTstpHandler = sigHandler;		// mySIGTSTP handler
		return 0;
	}
	}
	return 1;
}

// **********************************************************************
//	sigSignal - send signal to task(s)
//
//	taskId = task (-1 = all tasks)
//	sig = signal
//
int sigSignal(int taskId, int sig) {
	// check for task
	if ((taskId >= 0) && tcb[taskId].name) {
		debugPrint('s', 'f', "sigSignal(%i, %04x)\n", taskId, sig);
		tcb[taskId].signal |= sig;
		return 0;
	} else if (taskId == -1) {
		debugPrint('s', 'f', "sigSignal(%i, %04x)\n", taskId, sig);
		for (taskId = 0; taskId < MAX_TASKS; taskId++) {
			sigSignal(taskId, sig);
		}
		return 0;
	}
	// error
	return 1;
}

// **********************************************************************
//	clearSignal - clear signal for task(s)
//
//	taskId = task (-1 = all tasks)
//	sig = signal
//
int clearSignal(int taskId, int sig) {
	// check for task
	if ((taskId >= 0) && tcb[taskId].name) {
		debugPrint('s', 'f', "sigSignal(%i, %04x)\n", taskId, ~sig);
		tcb[taskId].signal &= ~sig;
		return 0;
	} else if (taskId == -1) {
		debugPrint('s', 'f', "sigSignal(%i, %04x)\n", taskId, ~sig);
		for (taskId = 0; taskId < MAX_TASKS; taskId++) {
			clearSignal(taskId, sig);
		}
		return 0;
	}
	// error
	return 1;
}

// **********************************************************************
// **********************************************************************
//	Default signal handlers
//

void defaultSigContHandler(void)			// task mySIGCONT handler
{

	debugPrint('s', 'f', "defaultSigContHandler()\n");
	return;
}

void defaultSigIntHandler(void)				// task mySIGINT handler
{
	debugPrint('s', 'f', "defaultSigIntHandler()\n");
	sigSignal(-1, mySIGTERM);
	return;
}

void defaultSigKillHandler(void)			// task mySIGKILL handler
{
	debugPrint('s', 'f', "defaultSigKillHandler()\n");
	return;
}

void defaultSigTermHandler(void)			// task mySIGTERM handler
{
	debugPrint('s', 'f', "defaultSigTermHandler()\n");
	killTask(curTask->tid);
	return;
}

void defaultSigTstpHandler(void)			// task mySIGTSTP handler
{
	debugPrint('s', 'f', "defaultSigtstpHandler()\n");
	sigSignal(-1, mySIGSTOP);
	return;
}

void createTaskSigHandlers(int tid) {
	debugPrint('s', 'f', "createTaskSigHandlers()");
	tcb[tid].signal = 0;
	if (tid) {
		// inherit parent signal handlers
		tcb[tid].sigContHandler = tcb[curTask->tid].sigContHandler;// mySIGCONT handler
		tcb[tid].sigIntHandler = tcb[curTask->tid].sigIntHandler;// mySIGINT handler
		tcb[tid].sigKillHandler = tcb[curTask->tid].sigKillHandler;// mySIGKILL handler
		tcb[tid].sigTermHandler = tcb[curTask->tid].sigTermHandler;// mySIGTERM handler
		tcb[tid].sigTstpHandler = tcb[curTask->tid].sigTstpHandler;// mySIGTSTP handler

	} else {
		// otherwise use defaults
		tcb[tid].sigContHandler = defaultSigContHandler;// task mySIGCONT handler
		tcb[tid].sigIntHandler = defaultSigIntHandler;	// task mySIGINT handler
		tcb[tid].sigKillHandler = defaultSigKillHandler;// task mySIGKILL handler
		tcb[tid].sigTstpHandler = defaultSigTstpHandler;// task mySIGTSTP handler
		tcb[tid].sigTermHandler = defaultSigTermHandler;// task mySIGTERM handler
	}
}
