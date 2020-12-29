#ifndef PROCESS
#define PROCESS
int pipefd[3][2];
int process1(const char *fifo1Loc);
int process2(const char *fifo1Loc,const char *fifo2Loc);
int process3(const char *fifo2Loc);

#endif