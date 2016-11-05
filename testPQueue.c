#include <stdio.h>

#include "pqueue.h"

int mainTest(int argc, char* argv[]) {
	printf("Starting Test Main\n");
	Pqueue* pq = newPqueue(5);
	printPqueue(pq);
	put(pq, newPqEntry(0, 1));
	printPqueue(pq);
	put(pq, newPqEntry(1, 1));
	printPqueue(pq);
	put(pq, newPqEntry(2, 1));
	printPqueue(pq);
	put(pq, newPqEntry(3, 1));
	printPqueue(pq);
	put(pq, newPqEntry(4, 1));
	printPqueue(pq);
	put(pq, newPqEntry(5, 2));
	printPqueue(pq);
	put(pq, newPqEntry(6, 1));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	put(pq, newPqEntry(7, 3));
	printPqueue(pq);
	put(pq, newPqEntry(8, 1));
	printPqueue(pq);
	put(pq, newPqEntry(9, 2));
	printPqueue(pq);
	put(pq, newPqEntry(10, 3));
	printPqueue(pq);
	put(pq, newPqEntry(11, 2));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
	free(pop(pq));
	printPqueue(pq);
}
