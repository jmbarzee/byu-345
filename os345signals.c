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
#include "pq.h"

extern TCB tcb[];							// task control block

// ***********************************************************************
// ***********************************************************************
//	Call all pending task signal handlers
//
//	return 1 if task is NOT to be scheduled.
//
int signals(void) {
	//debugPrint('s', 'f', "signals()\n");
	int ret = 0;
	if (tcb[getCurTask()].signal) {
		debugPrint('s', ' ', "check task: %i signal: %04x\n", getCurTask(),
				tcb[getCurTask()].signal);
		if (tcb[getCurTask()].signal & mySIGCONT) {
			tcb[getCurTask()].signal &= ~mySIGCONT;
			(*tcb[getCurTask()].sigContHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGINT) {
			tcb[getCurTask()].signal &= ~mySIGINT;
			(*tcb[getCurTask()].sigIntHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGKILL) {
			tcb[getCurTask()].signal &= ~mySIGKILL;
			(*tcb[getCurTask()].sigKillHandler)();
		}
		if (tcb[getCurTask()].signal & mySIGTERM) {
			tcb[getCurTask()].signal &= ~mySIGTERM;
			(*tcb[getCurTask()].sigTermHandler)();
			ret = 1;
		}
		if (tcb[getCurTask()].signal & mySIGTSTP) {
			tcb[getCurTask()].signal &= ~mySIGTSTP;
			(*tcb[getCurTask()].sigTstpHandler)();
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
		tcb[getCurTask()].sigContHandler = sigHandler;		// mySIGCONT handler
		return 0;
	}
	case mySIGINT: {
		tcb[getCurTask()].sigIntHandler = sigHandler;		// mySIGINT handler
		return 0;
	}
	case mySIGKILL: {
		tcb[getCurTask()].sigKillHandler = sigHandler;		// mySIGKILL handler
		return 0;
	}
	case mySIGTERM: {
		tcb[getCurTask()].sigTermHandler = sigHandler;		// mySIGTERM handler
		return 0;
	}
	case mySIGTSTP: {
		tcb[getCurTask()].sigTstpHandler = sigHandler;		// mySIGTSTP handler
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
	killTask(getCurTask());
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
		tcb[tid].sigContHandler = tcb[getCurTask()].sigContHandler;// mySIGCONT handler
		tcb[tid].sigIntHandler = tcb[getCurTask()].sigIntHandler;// mySIGINT handler
		tcb[tid].sigKillHandler = tcb[getCurTask()].sigKillHandler;// mySIGKILL handler
		tcb[tid].sigTermHandler = tcb[getCurTask()].sigTermHandler;// mySIGTERM handler
		tcb[tid].sigTstpHandler = tcb[getCurTask()].sigTstpHandler;// mySIGTSTP handler

	} else {
		// otherwise use defaults
		tcb[tid].sigContHandler = defaultSigContHandler;// task mySIGCONT handler
		tcb[tid].sigIntHandler = defaultSigIntHandler;	// task mySIGINT handler
		tcb[tid].sigKillHandler = defaultSigKillHandler;// task mySIGKILL handler
		tcb[tid].sigTstpHandler = defaultSigTstpHandler;// task mySIGTSTP handler
		tcb[tid].sigTermHandler = defaultSigTermHandler;// task mySIGTERM handler
	}
}
