/*
 * pqueue.h
 *
 *  Created on: Nov 4, 2016
 *      Author: jbarzee
 */

#ifndef PQUEUE_H_
#define PQUEUE_H_

#define CAP_INCREASE 5

// !!!!! IMPORTANT !!!!!
// all TqEntry pointers must be freed or returned to the pqueue

int main(int argc, char* argv[]);

typedef struct pqEntry {
	int tid;
	int priority;
} PqEntry;

PqEntry* newPqEntry(int tid, int priority);

typedef struct pqueue {
	int size;
	int cap;
	PqEntry** content;
} Pqueue;

Pqueue* newPqueue(int cap);
PqEntry* pop(Pqueue* q);
void put(Pqueue* q, PqEntry* entry);
void printPqueue(Pqueue* q);



#endif /* PQUEUE_H_ */
