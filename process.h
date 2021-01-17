#ifndef PROCESS
#define PROCESS
#include <signal.h>
int *pipe_counter;
int pipefd[64][3][2];
int pipe_term[2];
int verbose;
sigset_t blocked,blocked_chld,blocked_run;
int process1(const char *fifo1Loc);
int process2(const char *fifo1Loc,const char *fifo2Loc);
int process3(const char *fifo2Loc);

#endif