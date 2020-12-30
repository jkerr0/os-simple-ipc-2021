#ifndef PROCESS
#define PROCESS
#include <signal.h>
int *pipefd[3];
sigset_t blocked,blocked_chld;
int process1(const char *fifo1Loc);
int process2(const char *fifo1Loc,const char *fifo2Loc);
int process3(const char *fifo2Loc);

#endif