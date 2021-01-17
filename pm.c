#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "process.h"
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
#define BSIZE 256

void main_handler(int signal, siginfo_t* info,void*);
pid_t child[3];



int main(int argc, char *argv[])
{
    if(argc==2 && strcmp(argv[1],"-v")==0) verbose=1;
    else verbose=0;

    //ustawienie maski sygnalow
    sigfillset(&blocked);
    sigdelset(&blocked,SIGTERM);
    sigdelset(&blocked,SIGTSTP);
    sigdelset(&blocked,SIGCONT);
    sigdelset(&blocked,SIGUSR1);
    sigdelset(&blocked,SIGUSR2);
    sigfillset(&blocked_chld);
    sigdelset(&blocked_chld,SIGUSR1);
    sigdelset(&blocked_chld,SIGUSR2);
    sigprocmask(SIG_SETMASK,&blocked,NULL);
    sigfillset(&blocked_run);
    sigdelset(&blocked_run,SIGUSR1);
    int run=1;
    //tworzenie fifo

    const char *fifoLoc[2]={"./fifo1","./fifo2"};
    
    for(int i=0;i<2;i++) 
    {
        unlink(fifoLoc[i]);
        mkfifo(fifoLoc[i],0666);
    }
    //ustawianie pipe
    int pCountID = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0666);
    if(pCountID==-1)
    {
        printf("Error shmget\n");
        return 4;
    }
    pipe_counter = (int*)shmat(pCountID,NULL,0);
    if(pipe_counter==NULL)
    {
        printf("Error shmat\n");
        return 5;
    }
    *pipe_counter=0;
    for(int a=0;a<64;a++)
    {
        for(int i=0;i<3;i++)
        {
            if(pipe(pipefd[a][i])==-1)
            {
                printf("Error pipe [%d][%d]\n",a,i);
            }
        }
    }
    if(pipe(pipe_term)==-1)
    {
        printf("Error - pipe_term\n");
    }
    //tworzenie procesow potomnych
    
    int process_ret[3]={};
    printf("PM: %d\n",getpid());
    for(int i=0; i<3; i++)
    {
        switch(child[i]=fork())
        {
            case 0:
            //potomek i
            
            switch(i)
            {
                case 0:
                //czyta z wejscia/pliku
                //proces 1
                    process_ret[i]=process1(fifoLoc[0]);
                break;
                case 1:
                    process_ret[i]=process2(fifoLoc[0],fifoLoc[1]);
                break;
                case 2:
                    process_ret[i]=process3(fifoLoc[1]);
                //wypisuje
                break; 

            }
            exit(process_ret[i]);
            case -1:
            //err
            printf("Fork %d error\n",i);
            exit(1);
        }
    }
    //macierzysty
    for(int i=0;i<3;i++) printf("P%d: %d\n",i+1,child[i]);
    struct sigaction sa_pm;
    sigfillset(&sa_pm.sa_mask);
    sa_pm.sa_flags=SA_SIGINFO|SA_RESTART;
    sa_pm.sa_sigaction = main_handler;
    sigaction(SIGTSTP,&sa_pm,NULL);
    sigaction(SIGTERM,&sa_pm,NULL);
    sigaction(SIGCONT,&sa_pm,NULL);
    sigaction(SIGUSR1,&sa_pm,NULL);
    for(int i=0;i<3;i++) wait(&process_ret[i]);
    int ret_val[3];
    shmdt(pipe_counter);

    for(int i=0;i<3;i++)
    {
        if(WIFEXITED(process_ret[i]))
        {
            ret_val[i]=WEXITSTATUS(process_ret[i]);
            if(ret_val[i]!=0) printf("Proces potomny %d zwrocil %d\n",i+1,ret_val[i]);
        }
    }
    return 0;
}

void main_handler(int signal,siginfo_t *info, void* ptr)
{
    pid_t senderPID = info->si_pid;
    const int sigRecv = signal;
    if(senderPID==child[2])
    {
        if(sigRecv!=SIGUSR1)
        {
            //printf("SenderPID=%d, signal=%d\n",senderPID,signal);
            for(int i=0; i<3; i++)
            {
                close(pipefd[*pipe_counter][i][0]);
                if(write(pipefd[*pipe_counter][i][1],&sigRecv,sizeof(int))==-1)
                {
                    printf("Pipe write error\n");
                }
                close(pipefd[*pipe_counter][i][1]);
            } 
            //printf("Wpisano do pipe\n");
            if(sigRecv==SIGTSTP || sigRecv==SIGCONT)
            kill(child[0],SIGUSR1);
            else if(sigRecv==SIGTERM) kill(child[0],SIGUSR2);
            //printf("SIGUSR1 wyslany\n");
        }
        else
        {
            printf("Wstrzymano\n");
            //execl("/bin/bash","");
            //kill(getppid(),SIGCONT);
            sigsuspend(&blocked);
        }
        
    }
}
