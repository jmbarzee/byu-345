/*
 * pqueue.c
 *
 *  Created on: Nov 4, 2016
 *      Author: jbarzee
 */

#include <stdio.h>

#include "pqueue.h"

// !!!!! IMPORTANT !!!!!
// all PqEntry pointers must be freed or returned to the pqueue

PqEntry* newPqEntry(int tid, int priority) {
	//printf("newPqEntry(%i, %i)\n", tid, priority);
	PqEntry* ret = malloc(sizeof(PqEntry));
	ret->tid = tid;
	ret->priority = priority;
	return ret;
}

Pqueue* newPqueue(int cap) {
	printf("newPqueue(%i)\n", cap);
	Pqueue* ret = malloc(sizeof(Pqueue));
	ret->size = 0;
	if (cap < 1) {
		ret->cap = 0;
		ret->content = 0;
	} else {
		ret->cap = cap;
		ret->content = malloc(sizeof(PqEntry)*(cap));
	}
	return ret;
}

PqEntry* pop(Pqueue* q) {
	printf("pop()");
	PqEntry* ret;
	if (q->size < 1) {
		ret = malloc(sizeof(PqEntry));
		ret->priority = -1;
		ret->tid = -1;
	} else {
		ret = q->content[q->size-1];
		q->content[q->size--] = 0;
	}
	printf("->  t: %i  p: %i\n", ret->tid, ret->priority);
	return ret;
}

void put(Pqueue* q, PqEntry* entry) {
	printf("put(t: %i  p: %i)\n", entry->tid, entry->priority);
	if (q->size == q->cap) {
		PqEntry** newContent = malloc(sizeof(PqEntry)*(q->cap + CAP_INCREASE));
		int newPos = q->size;
		while (newPos > -1) {
			if (newPos == 0) {
				newContent[newPos] = entry;
				break;
			} else {
				if (entry->priority > q->content[newPos-1]->priority) {
					newContent[newPos] = entry;
					for (newPos--; newPos > -1; newPos--) {
						newContent[newPos] = q->content[newPos];
					}
					break;
				} else {
					newContent[newPos] = q->content[newPos-1];
					q->content[newPos-1] = 0;
				}
			}
			newPos--;
		}
		if (q->content != 0) {
			free(q->content);
		}
		q->content = newContent;
		q->size++;
		q->cap += CAP_INCREASE;
	} else {
		int newPos = q->size;
		while (newPos > -1) {
			if (newPos == 0) {
				q->content[newPos] = entry;
				break;
			} else {
				if (entry->priority > q->content[newPos-1]->priority) {
					q->content[newPos] = entry;
					break;
				} else {
					q->content[newPos] = q->content[newPos-1];
					q->content[newPos-1] = 0;
				}
			}
			newPos--;
		}
		q->size++;
	}
}

void printPqueue(Pqueue* q) {
	printf("\n********* Pqueue *********\n");
	printf("cap:  %i\n", q->cap);
	printf("size: %i\n", q->size);
	for (int i = q->size-1; i > -1; i--) {
		printf("Pqueue[%i]: t: %i  p: %i\n", q->size-i-1, q->content[i]->tid, q->content[i]->priority);
	}
	printf("********* ****** *********\n\n");
}

