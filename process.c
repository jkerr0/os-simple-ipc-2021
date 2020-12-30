#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#define BSIZE 256

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define RUNNING 1
#define WAITING 0
//extern int sig_counter;
void p1_handler(int signal, siginfo_t *info, void* ptr);
int p1_status;


int menu()
{
    fputs("1.stdin\n",stdout);
    fputs("2.plik\n",stdout);
    fputs("Wybierz opcje:",stdout);
    int opcja;
    
    char op_buf[5]={};
    fflush(stdin);
    fflush(stdin);
    fflush(stdin);
    fgets(op_buf,5,stdin);
    opcja = atoi(op_buf);
    
    //scanf("%d",&opcja);
    return opcja;
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
    int pipe_signal;
    printf("P1 handler: senderPID=%d\n",senderPID);
    if(senderPID == getppid())
    {
        close(pipefd[0][1]);
        read(pipefd[0][0],&pipe_signal,sizeof(int));
        printf("Odczytany signal z pipe %d\n",pipe_signal);
        //while(p1_status==RUNNING);
        printf("SIGUSR1 do somsiada\n");
        kill(getpid()+1,SIGUSR1);
        close(pipefd[0][0]);
        switch(pipe_signal)
        {
            case SIGTERM:
                printf("P1 term\n");
                exit(0);
                break;
            case SIGTSTP:
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
    struct sigaction sa_p1;
    sigfillset(&sa_p1.sa_mask);
    sa_p1.sa_flags=SA_SIGINFO|SA_RESTART;
    sa_p1.sa_sigaction = p1_handler;
    sigaction(SIGUSR1,&sa_p1,NULL);

    sigset_t blockedSIG;
    sigfillset(&blockedSIG);
    while(run)
    {
        
        p1_status=WAITING;
        sigprocmask(SIG_SETMASK,&blocked_chld,NULL);
        fflush(stdin);
        option = menu();
        sigprocmask(SIG_SETMASK,&blockedSIG,NULL);
        p1_status=RUNNING;
        //sleep(1);
        int fifo1FD1;
        switch(option)
        {
            
            case 1:
                fflush(stdin);
                if((fifo1FD1 = open(fifo1Loc,O_WRONLY))==-1)
                {
                    printf("Fifo1 error - open\n");
                }
                while(line[0]!='.'){
                    fgets(line,BSIZE,stdin);
                    if(line[0]!='.' && write(fifo1FD1,line,BSIZE) == -1){
                        printf("blad przy zapisie buffera do fifo\n");
                        return 3; 
                    }
                }
                for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                if(write(fifo1FD1,line,BSIZE)==-1)
                {
                    printf("Error fifo1\n");
                }
                //zapis do pliku fifo
                //printf("Zapisano do fifo\n");
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
                        printf("Plik nie istnieje\n");
                    }
                    else
                    {
                        if((fifo1FD1 = open(fifo1Loc,O_WRONLY))==-1)
                        {
                            printf("Error fifo1 na open\n");
                        }
                        int readPossible=1;
                        while(readPossible)
                        {
                            sleep(1);
                            for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                            readPossible = readLine(fileIn,line);
                            //printf("read=%d line: %s \n",readPossible,line);
                            if(write(fifo1FD1,line,BSIZE)==-1)
                            {
                                printf("Error fifo1\n");
                            }
                            
                        }
                        fclose(fileIn);
                        close(fifo1FD1);
                    }
                } 
        break;
        default:
        printf("Error - niewlasciwa opcja: %d, %c\n",option,option);   
        }
        
    }
    return 0; 
}

void p2_handler(int signal, siginfo_t *info,void *ptr)
{
    pid_t senderPID = info->si_pid;
    int pipe_signal;
    printf("P2 handler %d from %d\n", signal, info->si_pid);
    if(senderPID == getpid()-1)
    {
        close(pipefd[1][1]);
        read(pipefd[1][0],&pipe_signal,sizeof(int));
        printf("Odczytany signal z pipe %d\n",pipe_signal);
        printf("SIGUSR1 do somsiada\n");
        kill(getpid()+1,SIGUSR1);
        close(pipefd[1][0]);
        switch(pipe_signal)
        {
            case SIGTERM:
                printf("P2 term\n");
                exit(0);
                break;
            case SIGTSTP:
                break;
            case SIGCONT:
                break;
        }  
    }
}

int process2(const char *fifo1Loc,const char *fifo2Loc)
{//odbiera od 0 i liczy il znakow
    sigprocmask(SIG_SETMASK,&blocked_chld,NULL);
    struct sigaction sa_p2;
    sa_p2.sa_flags = SA_SIGINFO;
    sa_p2.sa_sigaction = p2_handler;
    sigaction(SIGUSR1,&sa_p2,NULL);
    int run=1;
    while(run)
    {
        //sleep(1);
        int fifo1FD2 = open(fifo1Loc,O_RDONLY);
        int fifo2FD1 = open(fifo2Loc,O_WRONLY);
        if(fifo1FD2==-1 || fifo2FD1==-1)
        {
            printf("Error fifo1 or fifo2 - open\n");
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
                printf("FIFO1 error - read\n");
            }
            while(rdBuffer[charNum]!=0 && rdBuffer[charNum++]!='\n');
            //printf("Cnum: %d\n",charNum);
            if(write(fifo2FD1,&charNum,sizeof(int))==-1)
            {
                printf("FIFO2 error - write\n");
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
        switch(signal)
        {
            case SIGTSTP:
                printf("SIGTSTP\n");
                break;
            case SIGTERM:
                printf("SIGTERM\n");
                break;
            case SIGCONT:
                printf("SIGCONT\n");
                break;
        }
        kill(getppid(),signal);
    }
    else if(signal==SIGUSR1 && senderPID==getpid()-1)
    {
        pid_t senderPID = info->si_pid;
        int pipe_signal;
        printf("P3 handler %d from %d\n", signal, info->si_pid);
        close(pipefd[2][1]);
        read(pipefd[2][0],&pipe_signal,sizeof(int));
        printf("Odczytany signal z pipe %d\n",pipe_signal);
        close(pipefd[1][0]);
        for(int i=0;i<3;i++)
        {
            if(pipe(pipefd[i])==-1)
            {
                printf("Error pipe %d\n",i);
            }
        }
        switch(pipe_signal)
        {
            case SIGTERM:
                printf("P3 term\n");
                exit(0);
                break;
            case SIGTSTP:
                break;
            case SIGCONT:
                break;
        }  
    }  
}


int process3(const char *fifo2Loc)
{  
    int run=1;
    struct sigaction sa_p3,sa_old;
    sigprocmask(SIG_SETMASK,&blocked,NULL);
    sa_p3.sa_flags=SA_SIGINFO;
    sa_p3.sa_sigaction = p3_handler;
    sigaction(SIGTERM,&sa_p3,&sa_old);
    //sigaction(SIGTERM,&sa_old,NULL);
    sigaction(SIGTSTP,&sa_p3,NULL);
    sigaction(SIGCONT,&sa_p3,NULL);
    sigaction(SIGUSR1,&sa_p3,NULL);
    while(run)
    {
        //sleep(1);
        int fifo2FD2 = open(fifo2Loc,O_RDONLY);
        if(fifo2FD2==-1)
        {
            printf("Error fifo2 - open\n");
        }
        int charRecv=1;
        while(charRecv>0)
        {
            if(read(fifo2FD2,&charRecv,sizeof(int))==-1)
            {
                printf("FIFO2 error - read\n");
                break;
            }
            if(charRecv>0) printf("P3(%d):%d\n",getpid(),charRecv-1);
        }
        close(fifo2FD2);
    }
    return 0;
}