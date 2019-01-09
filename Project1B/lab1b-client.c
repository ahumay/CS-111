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

char * encryptionFile;
int logfd;
MCRYPT eType;

// as advised by TAs, structure for non canonical input mode with no echo based on:
// https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
void resetnoncanonical();
void setnoncanonical();
void ehandler(char indicator, int errorCode);
void encryptionSettings();
void logItem(char * id, char * info, size_t size);

int main(int argc, char ** argv){
///////////////////////// parse options
	int pflag = 0;
	int lflag = 0;
	char * logName;
	int portNum;
	int op;

	struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"log", required_argument, 0, 'l'},
		{"encrypt", required_argument, 0, 'e'},
		{0, 0, 0, 0}
	};

	while (1) {
		op = getopt_long(argc, argv, "", long_options, NULL);
		if (op == -1){
			break;
		}
		if (op == 'p'){
			pflag = 1;
			portNum = atoi(optarg);
			ehandler('p', portNum);
		} else if (op == 'l'){
			lflag = 1;
			logName = optarg;
		} else if (op == 'e'){
      		encryptionFile = optarg;
		} else {
			ehandler('a', -1);
		}
	}

	if (pflag == 0){ 
		ehandler('p', -1); 
	}

	if (encryptionFile){
	    encryptionSettings();
	}

	logfd = creat(logName, S_IRWXU);
	ehandler('c', logfd);

///////////////////////// set terminal
	setnoncanonical();

///////////////////////// connect to server
// structured heavily on: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c from spec
	int sockfd;
	struct sockaddr_in serverAddress;
	struct hostent * server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ehandler('s', sockfd);

	server = gethostbyname("localhost");

	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr.s_addr, (char *) server -> h_addr, server -> h_length);
	serverAddress.sin_port = htons(portNum);

	ehandler('b', connect(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)));

///////////////////////// read and write
	struct pollfd pollstatus[2];

	pollstatus[0].events = POLLIN | POLLHUP | POLLERR; // spec: wait for either input (POLLIN) or error (POLLHUP, POLLERR) events
	pollstatus[0].fd = STDIN_FILENO;
	pollstatus[1].events = POLLIN | POLLHUP | POLLERR; 
	pollstatus[1].fd = sockfd;

	char buffer[256];
	char toTerminal[256];
	char toServer[256];
	while (1) {
		ehandler('l', poll(pollstatus, 2, 0));
		int num_terminal_received = 0;
		int num_server_received = 0;

	//// read
		if (pollstatus[0].revents & POLLIN){
			num_terminal_received = read(pollstatus[0].fd, &buffer, 256);
			ehandler('r', num_terminal_received);
		}

		int terminal_char_num = 0;
		int server_char_num = 0;
		int i = 0;
		for (; i < num_terminal_received; i++) {
			char curChar = buffer[i];            
			if (curChar == '\003'){
				server_char_num++;
				toServer[server_char_num - 1] = curChar;
			} else if (curChar == '\004'){
				server_char_num++;
				toServer[server_char_num - 1] = curChar;
			} else if (curChar == '\r' || curChar == '\n'){
				server_char_num++;
				toServer[server_char_num - 1] = '\n';

				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = '\r';
				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = '\n';
			} else {
				server_char_num++;
				toServer[server_char_num - 1] = curChar;
				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = curChar;
			}
		}

		if (pollstatus[1].revents & POLLIN){
			num_server_received = read(pollstatus[1].fd, buffer, 256);
			ehandler('r', num_server_received);
			if (num_server_received == 0){
				exit(0);
			}
			if (lflag){
				logItem("RECEIVED", buffer, num_server_received);
			}
			if (encryptionFile){
				int x = 0;
				for (; x < num_server_received; x++) {
					mdecrypt_generic(eType, &buffer[x], 1);
				}
			}
		}

		int j = 0;
		for (; j < num_server_received; j++) {
			char curChar = buffer[j];
			if (curChar == '\003'){
				server_char_num++;
				toServer[server_char_num - 1] = curChar;
			} else if (curChar == '\004'){
				server_char_num++;
				toServer[server_char_num - 1] = curChar;
			} else if (curChar == '\n'){ // map received <cr> or <lf> into <cr><lf> 
				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = '\r';
				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = '\n';
			} else {
				terminal_char_num++;
				toTerminal[terminal_char_num - 1] = curChar;
			}
		}

	//// write
		if (terminal_char_num > 0){
			ehandler('w', write(STDOUT_FILENO, toTerminal, terminal_char_num));
		}
		if (server_char_num > 0){
			if (encryptionFile){
				int x = 0;
				for (; x < server_char_num; x++) {
					mcrypt_generic(eType, &toServer[x], 1);
				}
			}
			ehandler('w', write(sockfd, toServer, server_char_num));
			if (lflag){
				logItem("SENT", toServer, server_char_num);
			}
		}

	//// exit
		if (pollstatus[1].revents & POLLERR || pollstatus[1].revents & POLLHUP){
			exit(0);
		}
	}
	exit(1);
}

// based on: https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
struct termios cur_attributes;

void resetnoncanonical(){
	tcsetattr(STDIN_FILENO, TCSANOW, &cur_attributes); // tcsetattr - set the parameters associated with the terminal
}

void setnoncanonical(){
	struct termios newattr;
	tcgetattr(STDIN_FILENO, &cur_attributes); // tcgetattr - get the parameters associated with the terminal
	atexit(resetnoncanonical);
	tcgetattr(STDIN_FILENO, &newattr);   
	newattr.c_iflag = ISTRIP;
	newattr.c_oflag = 0;
	newattr.c_lflag = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr); // tcsetattr - set the parameters associated with the terminal
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

void logItem(char * id, char * info, size_t size){
	dprintf(logfd, "%s %li bytes: %s\n", id, size, info);
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
			fprintf(stderr, "\nFailure create()ing log file in client.\n");
		}
		exit(1);
	}
}