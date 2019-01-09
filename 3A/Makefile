# // NAME: Anthony Humay, Clayton Green
# // EMAIL: ahumay@ucla.edu, clayton.green26@gmail.com
# // ID: 304731856, 404546151

default: build

build: 
	gcc -Wall -Wextra -o lab3a lab3a.c

clean:
	rm -rf *.txt lab3a *.tar.gz *.csv

dist: build
	tar -czf lab3a-304731856.tar.gz lab3a.c Makefile README ext2_fs.h