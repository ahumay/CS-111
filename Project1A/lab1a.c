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

#include <termios.h>
#include <poll.h>
#include <sys/wait.h>

// as advised by TAs, structure for non canonical input mode with no echo based on:
// https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
void resetnoncanonical();
void setnoncanonical();
void ehandler(char indicator, int errorCode);

int main(int argc, char ** argv){
	int shell_flag = 0;
	char buffer[256];
	char curChar;

	setnoncanonical();

	struct option long_options[] = {
		{ "shell", required_argument, 0, 's'},
		{ 0, 0, 0, 0}
	};

	// again like in lab 0 based on https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	int op = getopt_long(argc, argv, "", long_options, 0);

	if (op == 's'){
		shell_flag = 1;
	} else if (op != -1){ // we allow no arguments, but not unknown arguments
		ehandler('a', -1);
	}

	///////////////////////// without --shell
	if (shell_flag == 0){
		while (1) {
			int num_char_received = read(STDIN_FILENO, &buffer, 256);
			if (num_char_received < 0){ ehandler('r', num_char_received); }
			int i = 0;
			for (; i < num_char_received; i++){
				curChar = buffer[i];
				if (curChar == '\r' || curChar == '\n'){ // map received <cr> or <lf> into <cr><lf> 
					char add[2] = "\r\n";
					int res = write(STDOUT_FILENO, &add, 2 * sizeof(char));
					ehandler('w', res);
				} else if (curChar == '\003' || curChar == '\004'){
					fprintf(stderr, "SHELL EXIT SIGNAL=0 STATUS=0\n");
					exit(0);
				} else {
					int res = write(STDOUT_FILENO, &curChar, sizeof(char));
					ehandler('w', res);
				}
			}
		}
	}

	///////////////////////// --shell
	int to_shell[2]; // pipe from terminal to shell 
	int from_shell[2]; // pipe from shell to terminal 
	struct pollfd pollstatus[2]; // from spec: Create an array of two pollfd structures, one describing the keyboard (stdin) and one describing the pipe that returns output from the shell
	pid_t rc = -999;
	if (shell_flag){
		int res = pipe(to_shell);
		ehandler('p', res);

		res = pipe(from_shell);
		ehandler('p', res);

		rc = fork();
		pollstatus[0].events = POLLIN | POLLHUP | POLLERR; // spec: wait for either input (POLLIN) or error (POLLHUP, POLLERR) events
		pollstatus[1].events = POLLIN | POLLHUP | POLLERR; 
		pollstatus[0].fd = STDIN_FILENO;
		pollstatus[1].fd = from_shell[0];
		if (rc == 0){ // child
			close(from_shell[0]);
			close(to_shell[1]);
			
			// spec: "standard input is a pipe from the terminal process"
			ehandler('d', dup2(to_shell[0], STDIN_FILENO));
			close(to_shell[0]);

			// spec: "standard output and standard error are (dups of) a pipe to the terminal process"
			ehandler('d', dup2(from_shell[1], STDOUT_FILENO));
			ehandler('d', dup2(from_shell[1], STDERR_FILENO));
			close(from_shell[1]);
			
			char * args[2]; // from man page: "The list of arguments must be terminated by a NULL pointer"
			args[0] = optarg;
			args[1] = NULL;
			ehandler('e', execvp(optarg, args));
		} else if (rc > 0){ // parent
			close(to_shell[0]);
			close(from_shell[1]);

			while (1){
				ehandler('l', poll(pollstatus, 2, 0));

				// Spec: "Always process all available input before processing the shut-down indication",
				// thus the ordering of
				// output from shell, input from terminal, shut-down

				// output from shell
				if (pollstatus[1].revents & POLLIN){
					int num_char_received = read(from_shell[0], &buffer, 256);
					if (num_char_received < 0){ ehandler('r', num_char_received); }
					int i = 0;
					for (; i < num_char_received; i++){
						curChar = buffer[i];
						if (curChar == '\r' || curChar == '\n'){ // map received <cr> or <lf> into <cr><lf> 
							char add[2] = "\r\n";
							int res = write(STDOUT_FILENO, &add, 2 * sizeof(char));
							ehandler('w', res);
						} else {
							int res = write(STDOUT_FILENO, &curChar, sizeof(char));
							ehandler('w', res);
						}
					}

				// input from terminal
				} else if(pollstatus[0].revents & POLLIN){
					int num_char_received = read(STDIN_FILENO, &buffer, 256);
					if (num_char_received < 0){ ehandler('r', num_char_received); }
					int i = 0;
					for (; i < num_char_received; i++){
						curChar = buffer[i];

						if (curChar == '\003'){
							ehandler('k', kill(rc, SIGINT));
						} else if (curChar == '\004'){
							close(to_shell[1]);
						} else if (curChar == '\r' || curChar == '\n'){ // map received <cr> or <lf> into <cr><lf> 
							char add[2] = "\r\n";
							int res = write(STDOUT_FILENO, &add, 2 * sizeof(char));
							ehandler('w', res); // Failure trying to write() input to terminal

							curChar = '\n';
							res = write(to_shell[1], &curChar, sizeof(char));
							ehandler('b', res); // Failure trying to write() input to shell
						} else {
							int res = write(STDOUT_FILENO, &curChar, sizeof(char));
							ehandler('w', res); // Failure trying to write() input to terminal

							res = write(to_shell[1], &curChar, sizeof(char));
							ehandler('b', res); // Failure trying to write() input to shell
						}
					}

				// shut-down
				} else if (pollstatus[1].revents & (POLLERR | POLLHUP)){
					close(from_shell[0]);
					int state = -1;
					waitpid(rc, &state, 0);
					ehandler('s', state);
					int signalNumber = WTERMSIG(state);
					int signalStatus = WEXITSTATUS(state);
					fprintf(stderr, "SHELL EXIT SIGNAL=%i STATUS=%i\n", signalNumber, signalStatus);
					exit(0);
				}
			}
		} else {
			ehandler('f', -1); // fork fail
		}
	} 
	exit(1);
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
			fprintf(stderr, "\nFailure trying to write() input to shell.\n");
		}
		if (indicator == 'c'){
			fprintf(stderr, "\nFailure trying to read() input to shell.\n");
		}
		if (indicator == 'p'){
			fprintf(stderr, "\nFailure trying to pipe().\n");
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
			fprintf(stderr, "\nFailure trying to waitpid() and shut down.\n");
		}
		exit(1);
	}
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