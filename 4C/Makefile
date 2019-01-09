#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856

default:
	gcc lab4c_tcp.c -Wextra -Wall -lmraa -lm -g -o lab4c_tcp
	gcc lab4c_tls.c -Wextra -Wall -lmraa -lm -lssl -lcrypto -g -o lab4c_tls

.SILENT:

clean:
	rm -rf lab4c_tcp *.o *.tar.gz LOGFILE; \
	rm -rf *.txt; \
	rm -rf lab4c-304731856.tar.gz functions.sh; \

dist:
	tar -czf lab4c-304731856.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README