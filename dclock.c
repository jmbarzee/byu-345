/*
 * dclock.c
 *
 *  Created on: Nov 13, 2016
 *      Author: jbarzee
 */

#include <stdlib.h>

#include "os345.h"
#include "dclock.h"

Memo* newMemo(int cost, Semaphore* task) {
	Memo* ret = malloc(sizeof(Memo));
	ret->cost = cost;
	ret->event = task;
	ret->next = 0;
	return ret;
}

void tick(Memo** dclock) {
	if (*dclock == 0) {
		return;
	}
	Memo* memo = *dclock;
	if (memo)
		memo->cost--;
	while (memo && memo->cost < 1) {
		semSignal(memo->event);
		Memo* deleteMe = memo;
		memo = memo->next;
		free(deleteMe);
	}
	(*dclock) = memo;
}

void insert(Memo** dclock, Memo* new) {
	Memo** next = dclock;
	while((*next) && (*next)->cost <= new->cost) {
		new->cost -= (*next)->cost;
		next = &((*next)->next);
	}
	if (*next) {
		(*next)->cost -= new->cost;
		new->next = (*next);
	}
	*next = new;
}
