#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856

default:
	gcc lab4b.c -Wextra -Wall -lmraa -lm -g -o lab4b

.SILENT:

check: clean default 
	./lab4b --period=2 --scale="F" --log="TESTLOG.txt" ; \
	sleep 5; \
	echo "OFF"; \
	if [ $$? -eq 0 ]; \
	then echo "SMOKE TEST SUCCESS - 1/3." ; \
	else echo "SMOKE TEST FAIL - Correct test did NOT complete." ; \
	fi;

	./lab4b --gibberish ; \
	if [ $$? -eq 1 ]; \
	then echo "SMOKE TEST SUCCESS - 2/3." ; \
	else echo "SMOKE TEST FAIL - Argument should have been recognized as invalid." ; \
	fi;

	./lab4b --scale=gibberish ; \
	if [ $$? -eq 1 ]; \
	then echo "SMOKE TEST SUCCESS - 3/3." ; \
	else echo "SMOKE TEST FAIL - --scale argument should have been recognized as invalid." ; \
	fi;

clean:
	rm -rf lab4b *.o *.tar.gz TESTLOG.txt LOGFILE; \
	rm -rf *.txt; \
	rm -rf lab4b-304731856.tar.gz functions.sh; \

dist:
	tar -czf lab4b-304731856.tar.gz lab4b.c Makefile README