#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856
#SLIPDAYS: 1

default:
	gcc lab1b-client.c -lmcrypt -Wall -Wextra -lz -o lab1b-client; \
	gcc lab1b-server.c -lmcrypt -Wall -Wextra -lz -o lab1b-server

client:
	gcc lab1b-client.c -lmcrypt -Wall -Wextra -lz -o lab1b-client
	
server:
	gcc lab1b-server.c -lmcrypt -Wall -Wextra -lz -o lab1b-server

.SILENT:

clean:
	rm -rf lab1b-server *.o *.tar.gz; \
	rm -rf lab1b-client *.o *.tar.gz; \
	rm functions.sh; \
	rm -rf *.txt 

dist:
	tar -czf lab1b-304731856.tar.gz lab1b-client.c lab1b-server.c my.key README Makefile 
