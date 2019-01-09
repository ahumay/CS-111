#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856
#SLIPDAYS: 1

default:
	gcc SortedList.c -pthread -lrt lab2_list.c -Wextra -Wall -g -o lab2_list -lprofiler

local:
	gcc SortedList.c -pthread lab2_list.c -Wextra -Wall -g -o lab2_list

.SILENT:

removeGraphs:
	rm lab2b_[1-5].png lab2b_list.csv

graphs:
	gnuplot lab2_list.gp

tests: default 
	for threadCount in 1 2 4 8 12 16 24; do \
		for itCount in 1000; do \
			./lab2_list --threads=$$threadCount --iterations=$$itCount --sync=m >> lab2b_list.csv; \
			./lab2_list --threads=$$threadCount --iterations=$$itCount --sync=s >> lab2b_list.csv; \
		done; \
	done

	@-for threadCount in 1 4 8 12 16; do \
		for itCount in 1 4 8 16; do \
			./lab2_list --threads=$$threadCount --iterations=$$itCount --yield=id --lists=4 >> lab2b_list.csv; \
		done; \
	done

	for threadCount in 1 4 8 12 16; do \
		for itCount in 10 20 40 80; do \
			./lab2_list --threads=$$threadCount --iterations=$$itCount --yield=id --lists=4 --sync=m >> lab2b_list.csv; \
			./lab2_list --threads=$$threadCount --iterations=$$itCount --yield=id --lists=4 --sync=s >> lab2b_list.csv; \
		done; \
	done

	for threadCount in 1 2 4 8 12; do \
		for listCount in 1 4 8 16; do \
			./lab2_list --threads=$$threadCount --iterations=1000 --lists=$$listCount --sync=m >> lab2b_list.csv; \
			./lab2_list --threads=$$threadCount --iterations=1000 --lists=$$listCount --sync=s >> lab2b_list.csv; \
		done; \
	done

profile:
	-rm profile.gperf 
	CPUPROFILE=profile.gperf ./lab2_list --threads=12 --sync=s --iterations=1000
	pprof --text lab2_list profile.gperf > profile.out
	pprof --list=threadWork lab2_list profile.gperf >> profile.out

clean:
	rm -rf lab2_list *.o *.tar.gz; \
	rm -rf *.txt; \
	rm -rf lab2b-304731856.tar.gz lab2_list SortedList.o functions.sh; \

dist: clean tests graphs
	tar -czf lab2b-304731856.tar.gz SortedList.c SortedList.h lab2_list.c profile.out lab2b_list.csv lab2b_[1-5].png lab2_list.gp Makefile README