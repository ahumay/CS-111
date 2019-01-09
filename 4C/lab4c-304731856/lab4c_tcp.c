// NAME: Anthony Humay
// EMAIL: ahumay@ucla.edu
// ID: 304731856
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <mraa.h>
#include <math.h>
#include <mraa/aio.h>

// 4c
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>

int STOP;
int SIZEint = sizeof(int);
int SIZEllong = sizeof(long long);
int SIZEcharptr = sizeof(char *);
int OFF;
int PERIOD;
int PORTNUM;
int logFlag = 0;
char * HOSTNAME = "";
char * UID = "";
char SCALE;
mraa_aio_context tempHardware;
mraa_gpio_context buttonHardware;
struct timeval ogTime;
FILE * logFD = 0;
FILE * curRead = 0;

void ehandler(char indicator, int errorCode);

double measureTemp(mraa_aio_context sensor);

void off();

void doCommand(char * command);

int main(int argc, char ** argv){
///////////////////////// parse options
	struct option long_options[] = {
		{"log", required_argument, 0, 'l'},
		{"scale", optional_argument, 0, 's'},
		{"period", optional_argument, 0, 'p'},
		{"id", required_argument, 0, 'i'},
		{"host", required_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int op = -1;
	while (1){
		op = getopt_long(argc, argv, "", long_options, NULL);
		if (op == -1){
			break;
		}
		if (op == 'p'){
			PERIOD = atoi(optarg);
		} else if (op == 'l'){
			logFlag = 1;
			logFD = fopen(optarg, "w+");
			if (logFD == NULL){ ehandler('w', -1); }
		} else if (op == 's'){
			if (strlen(optarg) > 1) {
				ehandler('a', -1);
			}
			if (optarg[0] == 'F' || optarg[0] == 'C'){
				SCALE = optarg[0];
			} else {
				ehandler('a', -1);
			}
		} else if (op == 'i'){
			UID = optarg;
		} else if (op == 'h'){
			HOSTNAME = optarg;
		} else {
			ehandler('a', -1);
		}
	}
	ehandler('a', PERIOD);
	ehandler('a', logFlag);

	tempHardware = mraa_aio_init(1);
	buttonHardware = mraa_gpio_init(60);
	mraa_gpio_dir(buttonHardware, MRAA_GPIO_IN);

	PORTNUM = atoi(argv[argc - 1]);
	STOP = 0;
	OFF = 0;
	PERIOD = 1;
	SCALE = 'F';

///////////////////////// connect to server
	struct sockaddr_in serverAddress;
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
	ehandler('s', socketFD);

	struct hostent * svrInfo = gethostbyname(HOSTNAME);
	if (svrInfo == NULL) { ehandler('h', -1); }

	// structured on: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c from 1B spec
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr.s_addr, (char *) svrInfo -> h_addr, svrInfo -> h_length);
	serverAddress.sin_port = htons(PORTNUM);
	ehandler('b', connect(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)));
///////////////////////// 

	char buffer[256];
	sprintf(buffer, "ID=%s\n", UID);
	dprintf(socketFD, "ID=%s\n", UID);
	if (logFlag){
		fprintf(logFD, "%s", buffer);
	}

	struct pollfd pollstatus[1];
	pollstatus[0].events = POLLIN | POLLHUP | POLLERR;
	pollstatus[0].fd = socketFD;

	struct timeval cur;
	gettimeofday(&ogTime, 0);
	while (1){
		OFF = mraa_gpio_read(buttonHardware) ? 1 : OFF;
		if (OFF){ off(); }

		gettimeofday(&cur, 0);
		int curPeriod = cur.tv_sec - ogTime.tv_sec;
		if (curPeriod >= PERIOD){
			double curTemp = measureTemp(tempHardware);
			struct tm * curTime = localtime(&ogTime.tv_sec);
			sprintf(buffer, "%02d:%02d:%02d %.1f\n", curTime -> tm_hour, curTime -> tm_min, curTime -> tm_sec, curTemp);
			dprintf(socketFD, "%02d:%02d:%02d %.1f\n", curTime -> tm_hour, curTime -> tm_min, curTime -> tm_sec, curTemp);
			fprintf(stdout, "%s", buffer);
			fflush(stdout);
			gettimeofday(&ogTime, 0);
			if (logFlag && !STOP){
				fprintf(logFD, "%s", buffer);
			}
		}

		int res = poll(pollstatus, 1, 0);
		ehandler('l', res);
		
		if (pollstatus[0].revents & POLLIN){
			char input[256];
			FILE * curRead = fdopen(socketFD, "r");
			fgets(input, 256, curRead);
			doCommand(input);
		}
	}

	mraa_aio_close(tempHardware);
	mraa_gpio_close(buttonHardware);

	exit(0);
}

void off(){
	char buffer[256];
	struct timeval cur;
	gettimeofday(&cur, 0);
	struct tm * curTime = localtime(&cur.tv_sec);
	sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", curTime -> tm_hour, curTime -> tm_min, curTime -> tm_sec);
	fprintf(stdout, "%s", buffer);
	if (logFlag){
		fprintf(logFD, "%s", buffer);
	}
	exit(0);
}

void doCommand(char * command){
	char buffer[256];
	strcpy(buffer, command);
	if (!strcmp(buffer, "OFF\n")) {
		fprintf(logFD, "OFF\n");
		fflush(logFD);
		off();
	} else if (!strcmp(buffer, "SCALE=F\n")) {
		SCALE = 'F';
	} else if (!strcmp(buffer, "SCALE=C\n")) {
		SCALE = 'C';
	} else if (!strcmp(buffer, "START\n")) {
		STOP = 0;
	} else if (!strcmp(buffer, "STOP\n")) {
		STOP = 1;
	} else {
		char * periodPrefix = "PERIOD=";
		char * logPrefix = "LOG ";
		buffer[7] = '\0';
		if (!strcmp(periodPrefix, buffer)) {
			PERIOD = atoi(&command[7]);
		} else {
			buffer[4] = '\0';
			if (strcmp(logPrefix, buffer) != 0) {
				fprintf(stdout, "ERROR: INVALID COMMAND %s\n", command);
				return;
			}
		}
	}

	if (logFlag){
		fprintf(logFD, "%s", command);
		fflush(logFD);
	}
}

double measureTemp(mraa_aio_context sensor) {
	const int B = 4275;
	const int R0 = 100000;

	int a = mraa_aio_read(sensor);

	float R = 1023.0 / a - 1.0;
	R = R0 * R;

	float temperature = 1.0 / (log(R/R0) / B + 1 / 298.15) - 273.15;

	if (SCALE == 'C'){
		return temperature;
	}
	return (temperature * 9 / 5) + 32;
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
		if (indicator == 'l'){
			fprintf(stderr, "\nFailure trying to poll().\n");
		}
		if (indicator == 'k'){
			fprintf(stderr, "\nFailure trying to OFF() shell.\n");
		}
		if (indicator == 's'){
			fprintf(stderr, "\nFailure making a socket() on the client.\n");
		}
		if (indicator == 'b'){
			fprintf(stderr, "\nFailure connecting() to server.\n");
		}
		if (indicator == 'h'){
			fprintf(stderr, "\nFailure to find a host.\n");
		}
		exit(1);
	}
}