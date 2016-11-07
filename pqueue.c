/*
 * pqueue.c
 *
 *  Created on: Nov 4, 2016
 *      Author: jbarzee
 */

#include <stdio.h>
#include <stdlib.h>

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

Pqueue* newPqueue(int cap, char* name) {
	//printf("newPqueue(%i, %s)\n", cap, name);
	Pqueue* ret = malloc(sizeof(Pqueue));
	ret->size = 0;
	ret->name = name;
	if (cap < 1) {
		ret->cap = 0;
		ret->content = 0;
	} else {
		ret->cap = cap;
		ret->content = malloc(sizeof(PqEntry)*(cap));
	}
	return ret;
}

void emptyPqueue(Pqueue* q) {
	//printf("emptyPqueue(%s)\n", q->name);
	for (int i = 0; i < q->size; i++) {
		free(q->content[i]);
		q->content[i] = 0;
	}
	q->size = 0;
}

PqEntry* pop(Pqueue* q) {
	//printf("pop(%s)", q->name);
	//fflush(stdout);
	PqEntry* ret;
	if (q->size < 1) {
		ret = malloc(sizeof(PqEntry));
		ret->priority = -1;
		ret->tid = -1;
	} else {
		ret = q->content[q->size-1];
		q->content[q->size--] = 0;
	}
	//printf(" ->  t: %i  p: %i\n", ret->tid, ret->priority);
	return ret;
}

PqEntry* eject(Pqueue* q, int tid) {
	//printf("eject(%s)", q->name);
	//fflush(stdout);
	PqEntry* ret = 0;
	int i;
	for (i = 0; i < q->size; i++) {
		if (q->content[i]->tid == tid) {
			ret = q->content[i];
			q->size--;
			break;
		}
	}
	while (i < q->cap-1) {
		q->content[i] = q->content[++i];
	}
	if (!ret) {
		ret = malloc(sizeof(PqEntry));
		ret->priority = -1;
		ret->tid = -1;
	}
	//printf(" ->  t: %i  p: %i\n", ret->tid, ret->priority);
	return ret;
}

void put(Pqueue* q, PqEntry* entry) {
	//printf("put(%s, t: %i  p: %i)\n", q->name, entry->tid, entry->priority);
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
	printf("name:  %i\n", q->name);
	printf("cap:  %i\n", q->cap);
	printf("size: %i\n", q->size);
	for (int i = q->size-1; i > -1; i--) {
		printf("Pqueue[%i]: t: %i  p: %i\n", q->size-i-1, q->content[i]->tid, q->content[i]->priority);
	}
	printf("********* ****** *********\n\n");
}

