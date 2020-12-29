$(CC)=gcc

all:
	make pm
	make process
	make program

pm: pm.c 
	gcc -c pm.c

process: process.c process.h
	gcc -c process.c process.h

program: pm.o process.o
	gcc pm.o process.o -o program

