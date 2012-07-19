inq: inq.o reporter.o
	gcc -std=gnu99 -pedantic -Wall -o inq -I/usr/local/include -L/usr/local/lib -lbluetooth inq.o reporter.o -lmongoc 

inq.o: inq.c
	gcc -std=gnu99 -c -pedantic -Wall -o inq.o inq.c 

reporter.o: reporter.c
	gcc -std=gnu99 -c -pedantic -Wall -o reporter.o reporter.c
