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
#define BSIZE 256

void main_handler(int signal, siginfo_t* info,void*);
pid_t child[3];



int main()
{
    //ustawienie maski sygnalow
    sigfillset(&blocked);
    sigdelset(&blocked,SIGTERM);
    sigdelset(&blocked,SIGTSTP);
    sigdelset(&blocked,SIGCONT);
    sigdelset(&blocked,SIGUSR1);
    sigfillset(&blocked_chld);
    sigdelset(&blocked_chld,SIGUSR1);
    sigprocmask(SIG_SETMASK,&blocked,NULL);
    int run=1;
    //tworzenie fifo

    const char *fifoLoc[2]={"./fifo1","./fifo2"};
    
    for(int i=0;i<2;i++) 
    {
        unlink(fifoLoc[i]);
        mkfifo(fifoLoc[i],0666);
    }
    //ustawianie pipe
    int pipeMemID;
    if((pipeMemID=shmget(IPC_PRIVATE,3*2*sizeof(int),IPC_CREAT|0666))==-1)
    {
        printf("Shm error\n");
    }
    pipefd[0]=(int*)shmat(pipeMemID,NULL,0);
    pipefd[1]=pipefd[0]+2;
    pipefd[2]=pipefd[1]+2;
    for(int i=0;i<3;i++)
    {
        if(pipe(pipefd[i])==-1)
        {
            printf("Error pipe %d\n",i);
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
    sigfillset(&sa_pm.sa_mask);
    sa_pm.sa_flags=SA_SIGINFO;
    sa_pm.sa_sigaction = main_handler;
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
        printf("SIGUSR1 wyslany\n");
    }
}
