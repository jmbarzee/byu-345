/*
 * pq.c
 *
 *  Created on: Nov 4, 2016
 *      Author: jbarzee
 */

#include <stdio.h>
#include <stdlib.h>

#include "os345.h"
#include "pq.h"

extern PQ* rQueue;						// task ready queue

void pqprintf(char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	//vprintf(fmt, args);
	fflush(stdout);
	va_end(args);
}

void printPqueue(PQ* q) {
	//pqprintf("********* Pqueue : %s *********\n", q->name);
	//pqprintf("cap:  %i\n", q->cap);
	//pqprintf("size: %i\n", q->size);
	for (int i = q->size - 1; i > -1; i--) {
		Tid tid = q->content[i];
		//pqprintf("PQ[%i]: t: %i  p: %i\n", q->size - i - 1, tid, taskPriority(tid));
	}
	//pqprintf("********* ****** *********\n");
}

PQ* newPqueue(int cap, char* name) {
	//pqprintf("newPqueue(%i, %s)\n", cap, name);
	PQ* ret = malloc(sizeof(PQ));
	ret->size = 0;
	ret->name = name;
	if (cap < 1) {
		ret->cap = 0;
		ret->content = 0;
	} else {
		ret->cap = cap;
		ret->content = malloc(sizeof(PQ) * (cap));
	}
	return ret;
}

Tid next(PQ* q) {
	//pqprintf("next(%s)\n", q->name);
	Tid tid = -1;
	if (q->size < 1)
		return -1;

	int pos = q->size - 1;
	tid = q->content[pos];
	int priority = taskPriority(tid);
	while (pos > 0) {
		if (priority > taskPriority(q->content[pos - 1]))
			break;
		q->content[pos] = q->content[pos - 1];
		pos--;
	}
	q->content[pos] = tid;
	//pqprintf("\t->  t: %i  p: %i\n", tid, taskPriority(tid));
	return tid;
}

Tid nextSlices(PQ* q) {
	//pqprintf("nextSlices(%s)\n", q->name);
	printPqueue(q);
	if (q->size < 1)
		return -1;

	Tid tid = -1;
	//for (int i = (q->size - 1); i >= 0; i--) {

	for (int i = 0; i < q->size; i++) {
		if (getSlices(q->content[i]) > 0) {
			tid = q->content[i];
			break;
		}
	}
	//pqprintf("\t->  t: %i  p: %i\n", tid, taskPriority(tid));
	return tid;
}

Tid pop(PQ* q) {
	//pqprintf("pop(%s)", q->name);
	Tid tid;
	if (q->size < 1) {
		tid = -1;
	} else {
		tid = q->content[q->size - 1];
		q->content[q->size--] = -1;
	}
	//pqprintf(" ->  t: %i  p: %i\n",tid, taskPriority(tid));
	return tid;
}

Tid pull(PQ* q, Tid tid) {
	//pqprintf("pull(%s, %i)", q->name, tid);
	Tid rtid = -1;
	int pos;
	for (pos = 0; pos < q->size; pos++) {
		if (q->content[pos] == tid) {
			rtid = q->content[pos];
			q->size--;
			break;
		}
	}
	while (pos < q->size) {
		q->content[pos] = q->content[pos + 1];
		pos++;
	}
	//pqprintf(" ->  t: %i  p: %i\n", rtid, taskPriority(rtid));
	return rtid;
}

void put(PQ* q, Tid tid) {
	int priority = taskPriority(tid);
	//pqprintf("put(%s, t: %i  p: %i)\n", q->name, tid, priority);
	int pos = q->size;
	if (q->size == q->cap) {
		Tid* newContent = malloc(sizeof(Tid) * (q->cap + CAP_INCREASE));
		while (pos > -1) {
			if (pos > 0) {
				if (priority > taskPriority(q->content[pos - 1])) {
					newContent[pos] = tid;
					for (pos--; pos > -1; pos--) {
						newContent[pos] = q->content[pos];
					}
					break;
				} else {
					newContent[pos] = q->content[pos - 1];
					q->content[pos - 1] = 0;
				}
			} else {
				newContent[pos] = tid;
			}
			pos--;
		}
		if (q->content != 0) {
			free(q->content);
		}
		q->content = newContent;
		q->cap += CAP_INCREASE;
	} else {
		while (pos > -1) {
			if (pos > 0) {
				if (priority > taskPriority(q->content[pos - 1])) {
					q->content[pos] = tid;
					break;
				} else {
					q->content[pos] = q->content[pos - 1];
					q->content[pos - 1] = 0;
				}
			} else {
				q->content[pos] = tid;
			}
			pos--;
		}
	}
	q->size++;
}

Tid ready(PQ* src, PQ* dst) {
	////pqprintf("ready(%s -> %s)\n", src->name, dst->name);
	Tid tid = pop(src);
	if (tid > -1)
		put(dst, tid);
	////pqprintf("\t->  t: %i  p: %i\n", tid, taskPriority(tid));
	return tid;
}

Tid block(PQ* src, PQ* dst, Tid tid) {
	////pqprintf("block(%s -> %s, t:%1i  p:%1i)\n", src->name, dst->name, tid, taskPriority(tid));
	Tid rtid = pull(src, tid);
	if (rtid > -1)
		put(dst, rtid);
	//return rtid;
}
