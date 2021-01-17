#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define BSIZE 256

void p1_handler(int signal, siginfo_t *info, void* ptr);
int p1_status;
extern int *pipe_counter;


void menu()
{
    fputs("1.stdin\n",stdout);
    fputs("2.plik\n",stdout);
    fputs("Wybierz opcje:\n",stdout);
}

int readLine(FILE *src, char *buffer)
{
    char newChar=0;
    int read=1;
    int i=0;
    while(newChar!='\n' && read!=0)
    {
        read = fread(&newChar,sizeof(char),1,src);
        /*if(newChar!='\n')*/buffer[i++]=newChar; //koment - nie liczy znaku \n
    }
    return read;
}

void p1_handler(int signal, siginfo_t *info, void* ptr)
{
    pid_t senderPID = info->si_pid;
    int pipe_signal=0;
    if(verbose) printf("P1 handler: senderPID=%d\n",senderPID);
    if(senderPID == getppid())
    {
        if(verbose)printf("P1 h: pc=%d\n",*pipe_counter);

        close(pipefd[*pipe_counter][0][1]);
        read(pipefd[*pipe_counter][0][0],&pipe_signal,sizeof(int));
        if(verbose) printf("Odczytany signal z pipe %d\n",pipe_signal);
        close(pipefd[*pipe_counter][0][0]);
        kill(getpid()+1,signal);
        switch(pipe_signal)
        {
            case SIGTERM:
               { //open pipe
                int ready;
                close(pipe_term[1]);
                read(pipe_term[0],&ready,sizeof(int));
                close(pipe_term[0]);
                //closing
                if(verbose) printf("From pipe_term: %d\n",ready);
                kill(getpid()+1,SIGUSR1);
                if(ready==1) exit(0);
                else printf("P1 handler error\n");
               }
                break;
            case SIGTSTP:
                sigsuspend(&blocked_chld);
                break;
            case SIGCONT:
                break;
        }  
    }
}

int process1(const char *fifo1Loc)
{
    char line[BSIZE];
    int option, run=1;
    //signal handler setup
    struct sigaction sa_p1;
    sigfillset(&sa_p1.sa_mask);
    sa_p1.sa_flags=SA_SIGINFO|SA_RESTART;
    sa_p1.sa_sigaction = p1_handler;
    sigaction(SIGUSR1,&sa_p1,NULL);
    sigaction(SIGUSR2,&sa_p1,NULL);

    while(run)
    {
        sigprocmask(SIG_SETMASK,&blocked_chld,NULL);
        fflush(stdin);
        fflush(stdout);
 
        int option;
        scanf("%d",&option);
        int fifo1FD1;
        switch(option)
        {
            
            case 1:
                //fflush(stdin);
                sigprocmask(SIG_SETMASK,&blocked_run,NULL);
                if((fifo1FD1 = open(fifo1Loc,O_WRONLY))==-1)
                {
                    printf("P1: Fifo1 error - open\n");
                }
                while(line[0]!='.'){
                    fgets(line,BSIZE,stdin);
                    if(line[0]!='.' && write(fifo1FD1,line,BSIZE) == -1){
                        printf("P1: blad przy zapisie buffera do fifo\n");
                        return 3; 
                    }
                }
                for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                if(write(fifo1FD1,line,BSIZE)==-1)
                {
                    printf("P1: Error fifo1\n");
                }
                close(fifo1FD1);
            break;
            case 2:
                {
                    char fileName[FILENAME_MAX];
                    printf("Sciezka do pliku: ");
                    scanf("%s",fileName);
                    FILE *fileIn = fopen(fileName,"r");
                    if(fileIn==NULL)
                    {
                        printf("P1: Plik nie istnieje\n");
                    }
                    else
                    {
                        sigprocmask(SIG_SETMASK,&blocked_run,NULL);
                        if((fifo1FD1 = open(fifo1Loc,O_WRONLY))==-1)
                        {
                            printf("P1: Error fifo1 na open\n");
                        }
                        int readPossible=1;
                        while(readPossible)
                        {
                            for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                            readPossible = readLine(fileIn,line);
                            //printf("read=%d line: %s \n",readPossible,line);
                            if(write(fifo1FD1,line,BSIZE)==-1)
                            {
                                printf("P1: Error fifo1\n");
                            }
                            
                        }
                        close(fifo1FD1);
                        fclose(fileIn);
                    }
                } 
            break;
            default:
            printf("P1: Error - niewlasciwa opcja: %d\n",option/*,op_buf*/); 
            fflush(stdin);  
        }
        
    }
    return 0; 
}

void p2_handler(int signal, siginfo_t *info,void *ptr)
{
    pid_t senderPID = info->si_pid;
    int pipe_signal;
    if(verbose)
    {
    printf("P2 handler %d -> %d\n", signal, info->si_pid);
    printf("P2 h: pc=%d\n",*pipe_counter);
    }
    if(senderPID == getpid()-1)
    {
        kill(getpid()+1,signal);
        if(verbose)printf("Sig: %d -> %d\n",signal,getpid()+1);
        if(signal==SIGUSR1)
        {
            close(pipefd[*pipe_counter][1][1]);
            read(pipefd[*pipe_counter][1][0],&pipe_signal,sizeof(int));
            if(verbose)
            {
                printf("Odczytany signal z pipe %d\n",pipe_signal);
                printf("SIGUSR1 do %d\n",getpid()+1);
            }
            close(pipefd[*pipe_counter][1][0]);
            switch(pipe_signal)
            {
                case SIGTERM:
                    if(verbose) printf("P2 term\n");
                    exit(0);
                    break;
                case SIGTSTP:
                    sigsuspend(&blocked_chld);
                    break;
                case SIGCONT:
                    break;
            } 
        } 
    }
}

int process2(const char *fifo1Loc,const char *fifo2Loc)
{//odbiera od 0 i liczy il znakow
    sigprocmask(SIG_SETMASK,&blocked_chld,NULL);
    struct sigaction sa_p2;
    sa_p2.sa_flags = SA_SIGINFO|SA_RESTART;
    sa_p2.sa_sigaction = p2_handler;
    sigaction(SIGUSR1,&sa_p2,NULL);
    sigaction(SIGUSR2,&sa_p2,NULL);
    int run=1;
    while(run)
    {
        //sleep(1);
        int fifo1FD2 = open(fifo1Loc,O_RDONLY);
        int fifo2FD1 = open(fifo2Loc,O_WRONLY);
        if(fifo1FD2==-1 || fifo2FD1==-1)
        {
            printf("P2: Error fifo1 or fifo2 - open\n");
        }
        else
        {
        char rdBuffer[BSIZE];
        int charNum=1;
        while(charNum>0) //wlicza \n
        {
            //sleep(1);
            charNum=0;
            if(read(fifo1FD2,rdBuffer,BSIZE)==-1)
            {
                printf("P2: FIFO1 error - read\n");
            }
            while(rdBuffer[charNum]!=0 && rdBuffer[charNum++]!='\n');
            //printf("Cnum: %d\n",charNum);
            if(write(fifo2FD1,&charNum,sizeof(int))==-1)
            {
                printf("P2: FIFO2 error - write\n");
            }    
        }
        close(fifo1FD2);
        close(fifo2FD1);
        }
    }
    return 0;
}

void p3_handler(int signal, siginfo_t* info,void* ptr)
{
    pid_t senderPID = info->si_pid;
    if(senderPID!=getpid()-1 && senderPID!=getpid())
    {
        kill(getppid(),signal);
    }
    else if((signal==SIGUSR1 || signal==SIGUSR2) && senderPID==getpid()-1)
    {
        //printf("P3 h: pc=%d\n",*pipe_counter);
        if(signal==SIGUSR1)
        {
            int pipe_signal;
            if(verbose) printf("P3 handler %d from %d\n", signal, info->si_pid);
            
            close(pipefd[*pipe_counter][2][1]);
            read(pipefd[*pipe_counter][2][0],&pipe_signal,sizeof(int));
            
            if(verbose) printf("Odczytany signal z pipe %d\n",pipe_signal);
            
            close(pipefd[*pipe_counter][1][0]);
            *pipe_counter+=1;
            //printf("Pipe count: %d\n",*pipe_counter);
            switch(pipe_signal)
            {
                case SIGTERM:
                    if(verbose)printf("P3 term\n");
                    printf("\n");
                    exit(0);
                    break;
                case SIGTSTP:
                    kill(getppid(),SIGUSR1);
                    sigsuspend(&blocked);
                    break;
                case SIGCONT:
                    printf("Wznowiono\n");
                    //menu();
                    break;
            } 
        }
        else if(signal==SIGUSR2)
        {
            int ready=1;
            close(pipe_term[0]);
            write(pipe_term[1],&ready,sizeof(int));
            close(pipe_term[1]);
        }
    }  
}


int process3(const char *fifo2Loc)
{  
    int run=1;
    struct sigaction sa_p3,sa_old;
    
    sa_p3.sa_flags=SA_SIGINFO|SA_RESTART;
    sa_p3.sa_sigaction = p3_handler;
    sigaction(SIGTERM,&sa_p3,&sa_old);
    //sigaction(SIGTERM,&sa_old,NULL);
    sigaction(SIGTSTP,&sa_p3,NULL);
    sigaction(SIGCONT,&sa_p3,NULL);
    sigaction(SIGUSR1,&sa_p3,NULL);
    sigaction(SIGUSR2,&sa_p3,NULL);
    sigset_t blocked_run3 = blocked;
    sigaddset(&blocked_run3,SIGUSR2);

    while(run)
    {
        menu();
        int fifo2FD2 = open(fifo2Loc,O_RDONLY);
        if(fifo2FD2==-1)
        {
            printf("P3: Error fifo2 - open\n");
        }
        int charRecv=1;
        while(charRecv>0)
        {
            if(read(fifo2FD2,&charRecv,sizeof(int))==-1)
            {
                printf("P3 FIFO2 error - read\n");
                break;
            }
            sigprocmask(SIG_SETMASK,&blocked_run3,NULL);
            if(charRecv>0) printf("P3(%d):%d\n",getpid(),charRecv-1);
        } 
        close(fifo2FD2);
        sigprocmask(SIG_SETMASK,&blocked,NULL);
    } 
    return 0;
}