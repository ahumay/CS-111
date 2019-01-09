// NAME: Anthony Humay
// EMAIL: ahumay@ucla.edu
// ID: 304731856
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// 1a
#include <sys/wait.h>
#include <termios.h>
#include <poll.h>

// 1b
#include <sys/types.h>
#include <zlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <mcrypt.h>

// 2a
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include "SortedList.h"

pthread_mutex_t mutexLock;
int numIterations = 1;
int opt_yield = 0;
int numThreads = 1;
int syncLock = 0;
char testName[256] = "list";
char * syncType = "none";
SortedList_t * list;

void sigHandler(int errorCode);

void ehandler(char indicator, int errorCode);

void * threadWork(void * nodes);

void nullEntry();

void notFound();

void listNotEmpty();

int main(int argc, char ** argv) {
///////////////////////// parse options
	struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	int op = -1;
	while (1){
		op = getopt_long(argc, argv, "", long_options, NULL);
		if (op == -1){
			break;
		}
		if (op == 't'){
			numThreads = atoi(optarg);
		} else if (op == 'y'){
			int size = strlen(optarg);
			int i = 0;
			for (; i < size; i++) {
				if (optarg[i] == 'i'){
					opt_yield |= 1; // INSERT_YIELD
				} else if (optarg[i] == 'd'){
					opt_yield |= 2; // DELETE_YIELD
				} else if (optarg[i] == 'l'){
					opt_yield |= 4; // LOOKUP_YIELD
				} else {
					ehandler('a', -1);
				}
			}
		} else if (op == 'i'){
			numIterations = atoi(optarg);
		} else if (op == 's'){
			if (strcmp(optarg, "m") == 0){
				syncType = "m";
			} else if (strcmp(optarg, "s") == 0){
				syncType = "s";
			} else{
				ehandler('a', -1);
			}
		} else {
			ehandler('a', -1);
		}
	}

	ehandler('a', numThreads);
	ehandler('a', numIterations);

	signal(SIGSEGV, sigHandler);

	list = malloc(sizeof(SortedList_t));

	list -> next = NULL;
	list -> prev = NULL;
	list -> key = NULL;

	int ops = numThreads * numIterations;

	SortedListElement_t * listItems = malloc(sizeof(SortedListElement_t) * ops);

	int i = 0;
	for (; i < ops; i++) {
		char curKey[10];
		int j = 0;
		for (; j < 9; j++) {
			curKey[j] = (rand() % 32) + 6;
		}
		curKey[9] = '\0';
		listItems[i].key = curKey;
	}

	struct timespec startTime;
	clock_gettime(CLOCK_REALTIME, &startTime);

	pthread_t tids[numThreads];
	i = 0;
	for (; i < numThreads; i++) {
		pthread_create(&tids[i], NULL, threadWork, listItems + i * numIterations);
	}
	i = 0;
	for (; i < numThreads; i++) {
		pthread_join(tids[i], NULL);
	}

	struct timespec endTime;
	clock_gettime(CLOCK_REALTIME, &endTime);

	if (strcmp(syncType, "m") == 0){ pthread_mutex_lock(&mutexLock); }
	if (SortedList_length(list) > 0){ listNotEmpty(); }
	if (strcmp(syncType, "m") == 0){ pthread_mutex_unlock(&mutexLock); }

	int runTime = endTime.tv_nsec - startTime.tv_nsec;
	int numOperations = numThreads * numIterations * 3;
	long long int averageTime = runTime / numOperations;
	
	// yieldopts = {none, i,d,l,id,il,dl,idl}
	if (opt_yield == 0){
		strcat(testName, "-none");
	} else if (opt_yield == 1){
		strcat(testName, "-i");
	} else if (opt_yield == 2){
		strcat(testName, "-d");
	} else if (opt_yield == 3){
		strcat(testName, "-id");
	} else if (opt_yield == 4){
		strcat(testName, "-l");
	} else if (opt_yield == 5){
		strcat(testName, "-il");
	} else if (opt_yield == 6){
		strcat(testName, "-dl");
	} else if (opt_yield == 7){
		strcat(testName, "-idl");
	} else {
		exit(2);
	}

	strcat(testName, "-");
	strcat(testName, syncType);

	printf("%s%s%d%s%d%s%d%s%d%s%d\n", testName, ",", numThreads, ",", numIterations, ",1,", numOperations, ",", runTime, ",", (int)averageTime);
	exit(0);
}

void * threadWork(void * nodes){						
	SortedListElement_t * elements = nodes;

///////////////////////// insert
	int i = 0;
	for (; i < numIterations; i++) {
		if (!strcmp(syncType, "s")){
			while (__sync_lock_test_and_set(&syncLock, 1)){}
		} else if (!strcmp(syncType, "m")){
			pthread_mutex_lock(&mutexLock);
		} 

		SortedList_insert(list, &elements[i]);

		if (!strcmp(syncType, "s")){
			__sync_lock_release(&syncLock);
		} else if (!strcmp(syncType, "m")){
			pthread_mutex_unlock(&mutexLock);
		}
	}

///////////////////////// length
	if (!strcmp(syncType, "s")){
		while (__sync_lock_test_and_set(&syncLock, 1)){}
	} else if (!strcmp(syncType, "m")){
		pthread_mutex_lock(&mutexLock);
	} 

	if (SortedList_length(list) == -1) { nullEntry(); }

	if (!strcmp(syncType, "s")){
		__sync_lock_release(&syncLock);
	} else if (!strcmp(syncType, "m")){
		pthread_mutex_unlock(&mutexLock);
	}

///////////////////////// delete
	i = 0;
	for (; i < numIterations; i++) {
		if (!strcmp(syncType, "s")){
			while (__sync_lock_test_and_set(&syncLock, 1)){}
		} else if (!strcmp(syncType, "m")){
			pthread_mutex_lock(&mutexLock);
		} 

		SortedListElement_t * cur = SortedList_lookup(list, elements[i].key);
		if (cur == NULL) { notFound(); }
		if (SortedList_delete(cur) == 1) { nullEntry(); }

		if (!strcmp(syncType, "s")){
			__sync_lock_release(&syncLock);
		} else if (!strcmp(syncType, "m")){
			pthread_mutex_unlock(&mutexLock);
		}
	}
	pthread_exit(0);
}

void sigHandler(int errorCode){
	if (errorCode == SIGSEGV) {
		fprintf(stderr, "\nSegmentation fault.\n");
		exit(2);
	}
}

void listNotEmpty(){
	fprintf(stderr, "\nList is not empty.\n");
	exit(2);
}

void nullEntry(){
	fprintf(stderr, "\nPrev / next pointer is NULL\n");
	exit(2);
}

void notFound(){
	fprintf(stderr, "\nElement not found.\n");
	exit(2);
}

void ehandler(char indicator, int errorCode){
	if (errorCode < 0){
		if (indicator == 'a'){
			fprintf(stderr, "\nUnknown argument.\n");
		}
		if (indicator == 'w'){
			fprintf(stderr, "\nFailure write()ing.\n");
		}
		if (indicator == 'r'){
			fprintf(stderr, "\nFailure read()ing.\n");
		}
		if (indicator == 'd'){
			fprintf(stderr, "\nFailure trying to dup2() a FD.\n");
		}
		if (indicator == 'e'){
			fprintf(stderr, "\nFailure trying to execvp() the inputted program.\n");
		}
		if (indicator == 'f'){
			fprintf(stderr, "\nFailure trying to fork().\n");
		}
		if (indicator == 'l'){
			fprintf(stderr, "\nFailure trying to poll().\n");
		}
		if (indicator == 'k'){
			fprintf(stderr, "\nFailure trying to kill() shell.\n");
		}
		if (indicator == 's'){
			fprintf(stderr, "\nFailure making a socket() on the client.\n");
		}
		if (indicator == 'g'){
			fprintf(stderr, "\nFailure finding a host.\n");
		}
		if (indicator == 'p'){
			fprintf(stderr, "\nInvalid port.\n");
		}
		if (indicator == 'b'){
			fprintf(stderr, "\nFailure connecting() to server.\n");
		}
		if (indicator == 'c'){
			fprintf(stderr, "\nInvalid usage: --threads and --numIterations must be both given and positive numbers.\n");
		}
		exit(1);
	}
}