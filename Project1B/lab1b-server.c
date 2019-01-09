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
#include <mcrypt.h>

pid_t rc = -999;
char * encryptionFile;
int fd_socket = -1;
int processExitFlag = 0; 
int logfd;
MCRYPT eType;

void ehandler(char indicator, int errorCode);
void encryptionSettings();
void exitProcedure();

int main(int argc, char ** argv) {
///////////////////////// parse options
	int portNum;

	int op;

	struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"encrypt", required_argument, 0, 'e'},
		{0, 0, 0, 0}
	};

	while (1) {
		op = getopt_long(argc, argv, "", long_options, NULL);
		if (op == -1){
			break;
		}
		if (op == 'p'){
			portNum = atoi(optarg);
			ehandler('p', portNum);
		} else if (op == 'e'){
			encryptionFile = optarg;
		} else {
			ehandler('a', -1);
		}
	}

	if (encryptionFile){
		encryptionSettings();
	}

///////////////////////// create server
// structured heavily on: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server.c from spec
	int sockfd;
	socklen_t clientLength;
	struct sockaddr_in serverAddress, clientAddress;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ehandler('s', sockfd);
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(portNum);
	ehandler('b', bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)));
	listen(sockfd, 5);
	clientLength = sizeof(clientAddress);

	fd_socket = accept(sockfd, (struct sockaddr *) &clientAddress, &clientLength);
	ehandler('c', fd_socket);

///////////////////////// read and write
	char curChar;

	int to_shell[2];
	int from_shell[2];

	int res = pipe(to_shell);
	ehandler('g', res);
	res = pipe(from_shell);
	ehandler('g', res);

	rc = fork();

	struct pollfd pollstatus[2];
	pollstatus[0].fd = fd_socket;
	pollstatus[0].events = POLLIN | POLLERR | POLLHUP;
	pollstatus[1].fd = from_shell[0];
	pollstatus[1].events = POLLIN | POLLERR | POLLHUP;

	if (rc == 0){
		close(from_shell[0]);
		close(to_shell[1]);
		close(fd_socket);

		dup2(to_shell[0], 0);
		close(to_shell[0]);

		// from spec: input received from the shell pipes 
		// (which receive both stdout and stderr from the shell) should be forwarded out to the network socket:
		dup2(from_shell[1], 1); 
		dup2(from_shell[1], 2);
		close(from_shell[1]);

		char * args[2]; // from man page: "The list of arguments must be terminated by a NULL pointer"
		args[0] = NULL;
		ehandler('e', execvp("/bin/bash", args));
	} else if (rc > 0){
		atexit(exitProcedure);
		processExitFlag = 1;

		close(to_shell[0]);
		close(from_shell[1]);

		while (1){
			char buffer[256];

			ehandler('l', poll(pollstatus, 2, 0));

			// output from shell
			if (pollstatus[1].revents & POLLIN){
				int i = 0;
				int num_char_received = read(from_shell[0], &buffer, 256);
				if (num_char_received < 0){ ehandler('r', num_char_received); }

				if (encryptionFile){
					int x = 0;
					for (; x < num_char_received; x++) {
						mcrypt_generic(eType, &buffer[x], 1);
					}
				}

				for (; i < num_char_received; i++){
						curChar = buffer[i];
						if (curChar == '\004'){ // map received <cr> or <lf> into <cr><lf> 
							exit(0);
						} 
				}
				write(fd_socket, buffer, num_char_received);

			// input from terminal
			} else if (pollstatus[0].revents & POLLIN){
				int num_char_received = read(fd_socket, &buffer, 256);
				ehandler('r', num_char_received);
				if (encryptionFile){
					int x = 0;
					for (; x < num_char_received; x++) {
						mdecrypt_generic(eType, &buffer[x], 1);
					}
				}

				int i = 0;
				for (; i < num_char_received; i++){
					curChar = buffer[i];

					if (curChar == '\003'){
						ehandler('k', kill(rc, SIGINT));
					} else if (curChar == '\004'){
						close(to_shell[0]);
					} else if (curChar == '\r' || curChar == '\n'){ // map received <cr> or <lf> into <cr><lf> 
						curChar = '\n';
						res = write(to_shell[1], &curChar, sizeof(char));
						ehandler('b', res);
					} else {
						res = write(to_shell[1], &buffer[i], sizeof(char));
						ehandler('b', res);
					}
				}
			}

			// shut-down
			else if (pollstatus[1].revents & POLLHUP || pollstatus[1].revents & POLLERR || pollstatus[0].revents & POLLHUP || pollstatus[0].revents & POLLERR) {
				exit(0);
			}
		}
		processExitFlag = 0;
	} else {
		ehandler('f', -1); // fork fail
	}
	exit(1);
}


void encryptionSettings(){
	char * key;
	char * tempIV;
	int keyfd;

	eType = mcrypt_module_open("twofish", NULL, "cfb", NULL);
	if (eType == MCRYPT_FAILED){
		fprintf(stderr, "\nFailure using mcrypt_module_open().\n");
		exit(1);
	}

	keyfd = open(encryptionFile, O_RDONLY);
	key = malloc(16); 
	int size = mcrypt_enc_get_iv_size(eType);
	tempIV = malloc(size);
	bzero(key, 16);
	read(keyfd, key, 16);

	int i = 0;
	for(; i < size; i++){
		tempIV[i] = 0; 
	}

	mcrypt_generic_init(eType, key, 16, tempIV);

	for(; i < size; i++){
		tempIV[i] = i; 
	}

	close(keyfd);
}

void exitProcedure(){
	if (processExitFlag){
		shutdown(fd_socket, SHUT_RDWR);		
		int state = -1;
		waitpid(rc, &state, 0);
		ehandler('s', state);
		int signalNumber = WTERMSIG(state);
		int signalStatus = WEXITSTATUS(state);
		fprintf(stderr, "SHELL EXIT SIGNAL=%i STATUS=%i\n", signalNumber, signalStatus);
		exit(0);
	}
}

void ehandler(char indicator, int errorCode){
	if (errorCode < 0){
		if (indicator == 'a'){
			fprintf(stderr, "\nUnknown argument.\n");
		}
		if (indicator == 'w'){
			fprintf(stderr, "\nFailure trying to write() input to terminal.\n");
		}
		if (indicator == 'r'){
			fprintf(stderr, "\nFailure trying to read() input to terminal.\n");
		}
		if (indicator == 'b'){
			fprintf(stderr, "\nFailure bind()ing socket on the server.\n");
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
			fprintf(stderr, "\nFailure creating a socket() on the server.\n");
		}
		if (indicator == 'p'){
			fprintf(stderr, "\nInvalid port.\n");
		}
		if (indicator == 'c'){
			fprintf(stderr, "\nFailure accept()ing the socket on the server.\n");
		}
		if (indicator == 'g'){
			fprintf(stderr, "\nInvalid port.\n");
		}
		exit(1);
	}
}