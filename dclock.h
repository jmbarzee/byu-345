/*
 * dclock.h
 *
 *  Created on: Nov 13, 2016
 *      Author: jbarzee
 */

#ifndef DCLOCK_H_
#define DCLOCK_H_

#include "os345.h"

typedef struct memo{
	int cost;
	Semaphore* event;
	struct memo* next;
} Memo;

Memo* newMemo(int cost, Semaphore* task);

void tick(Memo** dclock);
void insert(Memo** dclock, Memo* new);

#endif /* DCLOCK_H_ */
