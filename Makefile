$(CC)=gcc

NAZWA = soproj

exec:
	make all
	./$(NAZWA)

all:
	make pm
	make process
	make $(NAZWA)

pm: pm.c 
	gcc -c pm.c

process: process.c process.h
	gcc -c process.c process.h

$(NAZWA): pm.o process.o
	gcc pm.o process.o -o $(NAZWA) -pthread

