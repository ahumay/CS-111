// NAME: Anthony Humay
// EMAIL: ahumay@ucla.edu
// ID: 304731856
// SLIPDAYS: 1

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

pthread_mutex_t * mutexLock;
int numIterations = 1;
int opt_yield = 0;
int numThreads = 1;
int * syncLock;
char testName[256] = "list";
char * syncType = "none";

SortedList_t ** list;
int listParts = 1;
int mutexFlag = 0;
int syncFlag = 0;
int listsFlag = 0;
long long * threadWaitTime;
int * walk;
int ops = 1;
SortedListElement_t * listItems;

int SIZEint = sizeof(int);
int SIZElist = sizeof(SortedList_t);
int SIZElistPTR = sizeof(SortedList_t *);
int SIZEllong = sizeof(long long);
int SIZEmutex = sizeof(pthread_mutex_t);
int SIZEelement = sizeof(SortedListElement_t);

void sigHandler(int errorCode);

void ehandler(char indicator, int errorCode);

void * threadWork(void * num);

void nullEntry();

void notFound();

void listNotEmpty();

void setUpWait();

void setUpSyncLock();

void setUpMutexLock();

void countWait();

long long findTimeDiff(struct timespec * start, struct timespec * end);

int main(int argc, char ** argv) {
///////////////////////// parse options
	struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{"lists", required_argument, 0, 'l'},
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
		} else if (op == 'i'){
			numIterations = atoi(optarg);
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
		} else if (op == 's'){
			if (strcmp(optarg, "m") == 0){
				syncType = "m";
				mutexFlag = 1;
			} else if (strcmp(optarg, "s") == 0){
				syncType = "s";
				syncFlag = 1;
			} else{
				ehandler('a', -1);
			}
		} else if (op == 'l'){
			listParts = atoi(optarg);
			listsFlag = 1;
		} else {
			ehandler('a', -1);
		}
	}

	ehandler('a', numThreads);
	ehandler('a', numIterations);
	ehandler('a', listParts);

	signal(SIGSEGV, sigHandler);

	setUpSyncLock();
	setUpMutexLock();

	list = (SortedList_t **) malloc(listParts * SIZElistPTR);
	int i;
	for (i = 0; i < listParts; i++){
		list[i] = (SortedList_t *) malloc(SIZElist);
		walk = 0;
		list[i] -> key = NULL;
		list[i] -> next = NULL;
		list[i] -> prev = NULL;
	}

	ops = numThreads * numIterations;

	listItems = malloc(SIZEelement * ops);
	for (i = 0; i < numThreads; i++){
		int j;
		for (j = 0; j < numIterations; j++){
			char * curKey = malloc(5);
			char param[4] = "%04d";
			int value = rand() % 10000;
			sprintf(curKey, param, value);
			listItems[(i * numIterations) + j].key = curKey;
		}
	}

	struct timespec startTime;
	clock_gettime(CLOCK_REALTIME, &startTime);

	setUpWait();

	int * index = malloc(numThreads * SIZEint);
	pthread_t tids[numThreads];
	i = 0;
	for (; i < numThreads; i++) {
		index[i] = i;
		pthread_create(&tids[i], NULL, threadWork, (void *) &index[i]);
	}

	i = 0;
	long long waitTime = 0;
	for (; i < numThreads; i++) {
		pthread_join(tids[i], NULL);
		waitTime += threadWaitTime[i];
	}

	struct timespec endTime;
	clock_gettime(CLOCK_REALTIME, &endTime);

	i = 0;
	for (; i < listParts; i++){
		if (mutexFlag){ pthread_mutex_lock(&mutexLock[i]); }
		if (SortedList_length(list[i]) > 0){ listNotEmpty(); }
		if (mutexFlag){ pthread_mutex_unlock(&mutexLock[i]); }
	}

	free(mutexLock);
	free(syncLock);
	free(threadWaitTime);
	free(index);

	long long int secondDiff = 1000000000 * (endTime.tv_sec - startTime.tv_sec);
	long long int nanoDiff = (endTime.tv_nsec - startTime.tv_nsec);
	long long int runTime = secondDiff + nanoDiff;
	int numOperations = numThreads * numIterations * 3;
	long long int averageTime = runTime / numOperations;
	int averageWaitTime = waitTime / (3 * ops);

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

	printf("%s%s%d%s%d%s%d%s%d%s%lld%s%d%s%d\n", testName, ",", numThreads, ",", numIterations, ",", listParts, ",", numOperations, ",", runTime, ",", (int)averageTime, ",", averageWaitTime);
	
	free(listItems);
	free(list);
	exit(0);
}

void setUpSyncLock(){
	int i;
	if (syncFlag){
		syncLock = malloc(listParts * SIZEint);
		for (i = 0; i < listParts; i++){
			syncLock[i] = 0;
		}
	}
}

void setUpMutexLock(){
	int i = 0;
	if (mutexFlag){
		mutexLock = malloc(listParts * SIZEmutex);
		for (i = 0; i < listParts; i++){
			pthread_mutex_init(&mutexLock[i], NULL);
		}
	}
}

void setUpWait(){
	threadWaitTime = malloc(numThreads * SIZEllong);
	int i;
	for (i = 0; i < numThreads; i++) {
		threadWaitTime[i] = 0;
	}
}

void * threadWork(void * num) {
	int place = * (int *) num;
	struct timespec threadStart;
	struct timespec threadEnd;

///////////////////////// insert
	int i = place;
	int listID = -1;
	for (; i < ops; i += numThreads){
		listID = atoi(listItems[i].key) % listParts;
		
		if (syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			while (__sync_lock_test_and_set(&syncLock[listID], 1)){}
		} else if (mutexFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			pthread_mutex_lock(&mutexLock[listID]);
		} 

		if (mutexFlag || syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadEnd);
			long long int secondDiff = 1000000000 * (threadEnd.tv_sec - threadStart.tv_sec);
			long long int nanoDiff = (threadEnd.tv_nsec - threadStart.tv_nsec);
			long long timeDiff = secondDiff + nanoDiff;
			threadWaitTime[place] += timeDiff;
		}

		SortedList_insert(list[listID], &listItems[i]);

		if (syncFlag){
			__sync_lock_release(&syncLock[listID]);
		} else if (mutexFlag){
			pthread_mutex_unlock(&mutexLock[listID]);
		}
	}

///////////////////////// length
	int totalLength = 0;
	i = 0;
	for (; i < listParts; i++){
		if (syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			while (__sync_lock_test_and_set(&syncLock[i], 1)){}
		} else if (mutexFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			pthread_mutex_lock(&mutexLock[i]);
		} 

		if (mutexFlag || syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadEnd);
			long long int secondDiff = 1000000000 * (threadEnd.tv_sec - threadStart.tv_sec);
			long long int nanoDiff = (threadEnd.tv_nsec - threadStart.tv_nsec);
			long long timeDiff = secondDiff + nanoDiff;
			threadWaitTime[place] += timeDiff;
		}

		int len = SortedList_length(list[i]);
		ehandler('n', len);
		totalLength += len;

		if (syncFlag){
			__sync_lock_release(&syncLock[i]);
		} else if (mutexFlag){
			pthread_mutex_unlock(&mutexLock[i]);
		}
	}

///////////////////////// delete
	i = 0;
	for (i = place; i < ops; i += numThreads){
		listID = atoi(listItems[i].key) % listParts;
		if (syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			while (__sync_lock_test_and_set(&syncLock[listID], 1)){}
		} else if (mutexFlag){
			clock_gettime(CLOCK_REALTIME, &threadStart);
			pthread_mutex_lock(&mutexLock[listID]);
		} 

		if (mutexFlag || syncFlag){
			clock_gettime(CLOCK_REALTIME, &threadEnd);
			long long int secondDiff = 1000000000 * (threadEnd.tv_sec - threadStart.tv_sec);
			long long int nanoDiff = (threadEnd.tv_nsec - threadStart.tv_nsec);
			long long timeDiff = secondDiff + nanoDiff;
			threadWaitTime[place] += timeDiff;
		}

		SortedListElement_t * cur = SortedList_lookup(list[listID], listItems[i].key);
		if (cur == NULL) { notFound(); }
		if (SortedList_delete(cur) == 1) { nullEntry(); }

		if (syncFlag){
			__sync_lock_release(&syncLock[listID]);
		} else if (mutexFlag){
			pthread_mutex_unlock(&mutexLock[listID]);
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
	fprintf(stderr, "\nPrev / next pointer is NULL.\n");
	exit(2);
}

void notFound(){
	fprintf(stderr, "\nElement not found.\n");
	exit(2);
}

long long findTimeDiff(struct timespec * start, struct timespec * end){
	long long int secondDiff = 1000000000 * (end -> tv_sec - start -> tv_sec);
	long long int nanoDiff = (end -> tv_nsec - start -> tv_nsec);
	long long timeDiff = secondDiff + nanoDiff;
    return timeDiff;
}

void ehandler(char indicator, int errorCode){
	if (errorCode < 0){
		if (indicator == 'a'){
			fprintf(stderr, "\nUnknown or invalid argument.\n");
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
		if (indicator == 'n'){
			fprintf(stderr, "\nNegative amount of elements in list.\n");
			exit(2);
		}
		exit(1);
	}
}