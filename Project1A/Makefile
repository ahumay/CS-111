#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856

default:
	gcc -g -Wall -Wextra lab1a.c -o lab1a

.SILENT:

clean:
	rm -rf lab1a *.o *.tar.gz; \
	rm -rf *.txt 

dist:
	tar -czf lab1a-304731856.tar.gz lab1a.c README Makefile 
