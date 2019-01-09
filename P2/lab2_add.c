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

pthread_mutex_t lock;
long long counter = 0;
int numThreads = 1;
int numIterations = 1;
int lockID = 0;
int spinlock_flag = 0;
int opt_yield = 0;
char testName[256] = "add";

void ehandler(char indicator, int errorCode);

void add(long long *pointer, long long val);

void * threadWork();

int main(int argc, char ** argv){
///////////////////////// parse options
	struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", no_argument, 0, 'y'},
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
		} else if (op == 'i'){
			numIterations = atoi(optarg);
		} else if (op == 'y'){
			opt_yield = 1;
		} else if (op == 's'){
			if (optarg[0] == 'm'){ // mutex corresponds to 1
				lockID = 1;
			} else if (optarg[0] == 's'){ // spin corresponds to 2
				lockID = 2;
			} else if (optarg[0] == 'c'){ // compare and swap corresponds to 3
				lockID = 3;
			} else {
				ehandler('a', -1);
			}
		} else {
			ehandler('a', -1);
		}
	}

	ehandler('c', numThreads);
	ehandler('c', numIterations);

	if (lockID == 1){
		pthread_mutex_init(&lock, NULL);
	}

	struct timespec startTime;
	clock_gettime(CLOCK_REALTIME, &startTime);

	pthread_t tids[numThreads];
	int i = 0;
	for (; i < numThreads; i++) {
		pthread_create(&tids[i], NULL, threadWork, NULL);
	}
	i = 0;
	for (; i < numThreads; i++) {
		pthread_join(tids[i], NULL);
	}

	struct timespec endTime;
	clock_gettime(CLOCK_REALTIME, &endTime);
	int runTime = endTime.tv_nsec - startTime.tv_nsec;
	int numOperations = numThreads * numIterations * 2;
	long long int averageTime = runTime / numOperations;

	if (opt_yield){
		strcat(testName, "-yield");
	}
	
	if (lockID == 0){
		strcat(testName, "-none");
	} else if (lockID == 1){
		strcat(testName, "-m");
	} else if (lockID == 2){
		strcat(testName, "-s");
	} else if (lockID == 3){
		strcat(testName, "-c");
	}

	printf("%s%s%d%s%d%s%d%s%d%s%d%s%lld\n", testName, ",", numThreads, ",", numIterations, ",", numOperations, ",", runTime, ",", (int)averageTime, ",", counter);
	
	exit(0);
}

void * threadWork(){
	int i;
	if (lockID == 1){
		i = 0;
		for (; i < numIterations; i++){
			pthread_mutex_lock(&lock);
			add(&counter, 1);
			pthread_mutex_unlock(&lock);
		}
	} else if (lockID == 2){
		i = 0;
		for (; i < numIterations; i++){
			while (__sync_lock_test_and_set(&spinlock_flag, 1));
			add(&counter, 1);
			__sync_lock_release(&spinlock_flag);
		}
	} else if (lockID == 3){
		i = 0;
		for (; i < numIterations; i++){
			long long temp = counter;
			long long updated = temp + 1;
			if (opt_yield){ 
				sched_yield();
			}
			while (__sync_val_compare_and_swap(&counter, temp, updated) != temp){
				temp = counter;
				updated = temp + 1;
				if (opt_yield){
					sched_yield();
				}
			}
		}
	} else if (lockID == 0){
		i = 0;
		for (; i < numIterations; i++){
			add(&counter, 1);
		}
	}  

	if (lockID == 1){
		i = 0;
		for (; i < numIterations; i++){
			pthread_mutex_lock(&lock);
			add(&counter, -1);
			pthread_mutex_unlock(&lock);
		}
	} else if (lockID == 2){
		i = 0;
		for (; i < numIterations; i++){
			while (__sync_lock_test_and_set(&spinlock_flag, 1));
			add(&counter, -1);
			__sync_lock_release(&spinlock_flag);
		}
	} else if (lockID == 3){
		i = 0;
		for (; i < numIterations; i++){
			long long temp = counter;
			long long updated = temp - 1;
			if (opt_yield){ 
				sched_yield();
			}
			while (__sync_val_compare_and_swap(&counter, temp, updated) != temp){
				temp = counter;
				updated = temp - 1;
				if (opt_yield){
					sched_yield();
				}
			}
		}
	} else if (lockID == 0){
		i = 0;
		for (; i < numIterations; i++){
			add(&counter, -1);
		}
	}
	return NULL;
}

void add(long long *pointer, long long val) {
    long long sum = *pointer + val;
    if (opt_yield){
    	sched_yield();
    }
    *pointer = sum;
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
			fprintf(stderr, "\nInvalid usage: --threads and --iterations must be both given and positive numbers.\n");
		}
		exit(1);
	}
}