#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856

default:
	gcc lab2_add.c -pthread -lrt -Wextra -Wall -o lab2_add; \
	gcc SortedList.c -pthread -lrt lab2_list.c -Wextra -Wall -g -o lab2_list

.SILENT:

graphs:
	gnuplot lab2_add.gp; \
	gnuplot lab2_list.gp
	
tests: default
	for threadCount in 1 2 4 8 12; do \
		for itCount in 100 1000 10000 100000; do \
			./lab2_add --threads=$$threadCount --iterations=$$itCount >> lab2_add.csv; \
		done ; \
	done

	for threadCount in 2 4 8 12; do \
		for itCount in 10 20 40 80 100 1000 10000 100000; do \
			./lab2_add --yield --threads=$$threadCount --iterations=$$itCount >> lab2_add.csv; \
		done ; \
	done

	for threadCount in 2 4 8 12; do \
		./lab2_add --yield --sync=s --threads=$$threadCount --iterations=1000 >> lab2_add.csv; \
		./lab2_add --yield --sync=m --threads=$$threadCount --iterations=10000 >> lab2_add.csv; \
		./lab2_add --yield --sync=c --threads=$$threadCount --iterations=10000 >> lab2_add.csv; \
	done

	for threadCount in 1 2 4 8 12 ; do \
		./lab2_add --sync=s --threads=$$threadCount --iterations=10000 >> lab2_add.csv; \
		./lab2_add --sync=m --threads=$$threadCount --iterations=10000 >> lab2_add.csv; \
		./lab2_add --sync=c --threads=$$threadCount --iterations=10000 >> lab2_add.csv; \
	done
	
	for itCount in 10 100 1000 10000 20000 ; do \
		./lab2_list --threads=1 --iterations=$$itCount >> lab2_list.csv; \
	done

	./lab2_list --yield=il --sync=m --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=il --sync=s --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=dl --sync=m --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=dl --sync=s --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=i --sync=m --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=i --sync=s --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=d --sync=m --threads=12 --iterations=32 >> lab2_list.csv
	./lab2_list --yield=d --sync=s --threads=12 --iterations=32 >> lab2_list.csv

	for threadCount in 1 2 4 8 12 16 24; do \
		./lab2_list --sync=m --threads=$$threadCount --iterations=1000 >> lab2_list.csv; \
		./lab2_list --sync=s --threads=$$threadCount --iterations=1000 >> lab2_list.csv; \
	done

clean:
	rm -rf lab2_add *.o *.tar.gz; \
	rm -rf lab2_list *.o; \
	rm -rf *.txt; \
	rm -rf lab2a-304731856.tar.gz lab2_add lab2_list SortedList.o functions.sh

dist: clean tests graphs
	tar -czf lab2a-304731856.tar.gz lab2_add.c SortedList.c SortedList.h lab2_list.c lab2_add.csv lab2_list.csv lab2_list-[1-4].png lab2_add-[1-5].png lab2_add.gp lab2_list.gp Makefile README