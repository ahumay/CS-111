#include <stdio.h> 
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void fault(); // create the segmentation fault

void handler(int signalNumber);

int main(int argc, char ** argv){
	int c; 
	int in = 0;
	int out = 1;
	int seg_flag = 0;
	int catch_flag = 0;

	// based on https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	struct option long_options[] = {
		{ "segfault", no_argument, 0, 's'},
		{ "catch", no_argument, 0, 'c'},
		{ "input", required_argument, 0, 'i'},
		{ "output", required_argument, 0, 'o'},
		{ 0, 0, 0, 0}
	};

	while (1){
		c = getopt_long(argc, argv, "", long_options, 0); // optString is empty since no specification on what is allowed in lab 
		if (c == -1){
			break; 
		} else if (c == 'i'){
			// from file descriptor manipulation attachment
			in = open(optarg, O_RDONLY);
			if (in >= 0){ 
				close(0);
				dup(in);
				close(in);
			} else {
				fprintf(stderr, "Error opening input file : %s. More details: %s \n", optarg, strerror(errno));
				exit(2);
			}
		} else if (c == 'o'){
			// from file descriptor manipulation attachment
			out = creat(optarg, 0666); // to create the file with name optarg
			if (out >= 0){
				close(1);
				dup(out);
				close(out);
			} else {
				fprintf(stderr, "Error opening output file : %s. More details: %s \n", optarg, strerror(errno));
				exit(3);
			}
		} else if (c == 's'){
			seg_flag = 1;
		} else if (c == 'c'){
			catch_flag = 1;
		} else {
			fprintf(stderr, "Error trying to recognize option: %s \n", argv[optind - 1]);
			exit(1);
		}
	}

	if (catch_flag == 1){
		signal(SIGSEGV, handler);
	}

	if (seg_flag == 1){
		fault(); // as advised in the spec
	}

	char * buffer = (char *) malloc(sizeof(char)); 
	while (read(0, buffer, 1) > 0){ // read one byte at a time
		write(1, buffer, 1); // write one byte at a time
	}
	free(buffer);

	exit(0);
}

void handler(int num){
	fprintf(stderr, "SIGSEGV handler caught a segmentation fault with signal number: %d \n", num);
	exit(4);
}

void fault() {
	int * p = NULL;
	*p = 4;
}