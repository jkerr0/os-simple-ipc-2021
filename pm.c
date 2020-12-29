#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "process.h"
#include <signal.h>
#define BSIZE 256

void main_handler(int signal, siginfo_t* info,void*);
pid_t child[3];



int main()
{
    int run=1;
    //tworzenie fifo

    const char *fifoLoc[2]={"./fifo1","./fifo2"};
    
    for(int i=0;i<2;i++) 
    {
        unlink(fifoLoc[i]);
        mkfifo(fifoLoc[i],0666);
    }
    //ustawianie pipe

    for(int i=0;i<3;i++)
    {
        if(pipe(pipefd[i])==-1)
        {
            printf("Error pipe %d",i);
        }
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
            printf("P%d: %d\n",i+1,getpid());
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
    struct sigaction sa_pm;
    sa_pm.sa_flags=SA_SIGINFO;
    sa_pm.sa_sigaction = main_handler;
    sigemptyset(&sa_pm.sa_mask);
    sigaction(SIGTSTP,&sa_pm,NULL);
    sigaction(SIGTERM,&sa_pm,NULL);
    sigaction(SIGCONT,&sa_pm,NULL);
    for(int i=0;i<3;i++) wait(&process_ret[i]);
    int ret_val[3];
    

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
    if(senderPID==child[2])
    {
        printf("SenderPID=%d, signal=%d\n",senderPID,signal);
        for(int i=0; i<3; i++)
        {
            close(pipefd[i][0]);
            if(write(pipefd[i][1],&signal,sizeof(int))==-1)
            {
                printf("Pipe write error\n");
            }
            close(pipefd[i][1]);
        } 
        printf("Wpisano do pipe\n");
        kill(child[0],SIGUSR1);
        printf("SIGUST1 wyslany\n");
    }
}
