default:
	gcc -g -Wall -Wextra lab0.c -o lab0

.SILENT:

exitTest:
	echo "Example 123" > t1.txt; \
	./lab0 --input=t1.txt --output=t2.txt ; \
	if [ $$? -eq 0 ]; then echo "SUCCESS - Test Case: program exits with 0" ; \
	else echo "FAIL - Test Case: program does not exit with 0" ; \
	fi;

argTest:
	./lab0 --gibberish ; \
	if [ $$? -eq 1 ]; \
	then echo "SUCCESS - Test Case: unknown arg exits with right code" ; \
	else echo "FAIL - Test Case: unknown arg exits with wrong code" ; \
	fi;

inputTest:
	./lab0 --input example123.txt ; \
	if [ $$? -eq 2 ]; \
	then echo "SUCCESS - Test Case: exits with right code when unable to open input file" ;\
	else echo "FAIL - Test Case: exits with wrong code when unable to open input file" ;\
	fi;

outputTest:
	echo "Example 123" > in.txt; \
	echo "" > out.txt; \
	chmod u-w out.txt; \
	./lab0 --input=in.txt --output=out.txt ; \
	if [ $$? -eq 3 ]; \
	then echo "SUCCESS - Test Case: exits with right code when unable to open output file"; \
	else echo "FAIL - Test Case: exits with wrong code when unable to open output file"; \
	fi; 

segTest:
	./lab0 --segfault --catch ; \
	if [ $$? -eq 4 ]; \
	then echo "SUCCESS - Test Case: segmentation catch exits with right code"; \
	else echo "FAIL - Test Case: segmentation catch exits with wrong code"; \
	fi;

clean:
	rm -rf lab0 *.o *.tar.gz

check: exitTest argTest inputTest outputTest segTest
	rm -rf *.txt 

dist:
	tar -czf lab0-304731856.tar.gz lab0.c backtrace.png breakpoint.png README Makefile 
