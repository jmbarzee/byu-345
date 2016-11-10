/*
 * pq.h
 *
 *  Created on: Nov 4, 2016
 *      Author: jbarzee
 */

#ifndef PQ_H_
#define PQ_H_

#include <stdarg.h>

#include "os345.h"

#define CAP_INCREASE 5

typedef int Tid;

typedef struct priorityQueue{
	int size;
	int cap;
	Tid* content;
	char* name;
} PQ;

PQ* newPqueue(int cap, char* name);

Tid pull(PQ* q, int tid);
Tid next(PQ* q);
Tid pop(PQ* q);

Tid nextSlices(PQ* q);

void put(PQ* q, Tid tid);

Tid ready(PQ* src, PQ* dst);
Tid block(PQ* src, PQ* dst, Tid tid);

void printPqueue(PQ* q);
void pqprintf(char* fmt, ...);



#endif /* PQ_H_ */
