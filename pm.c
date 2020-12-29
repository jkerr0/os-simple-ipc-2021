#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "process.h"


#define BSIZE 256

int main()
{
    int run=1;
    //tworzenie fifo
    const char *fifoLoc[2]={"./fifo1","./fifo2"};
    for(int i=0;i<2;i++) mkfifo(fifoLoc[i],0666);
    //ustawianie pipe

    //tworzenie procesow potomnych
    pid_t child[3];
    int process_ret[3]={};
    for(int i=0; i<3; i++)
    {
        switch(child[i]=fork())
        {
            case 0:
            //potomek i
            printf("%d %d\n",i,getpid());
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
    //macierz
    for(int i=0;i<3;i++) wait(&process_ret[i]);
    int ret_val[3];
    for(int i=0;i<3;i++)
    {
        if(WIFEXITED(process_ret[i]))
        {
            ret_val[i]=WEXITSTATUS(process_ret[i]);
            if(ret_val[i]!=0) printf("Proces potomny %d zwrocil %d\n",i,ret_val[i]);
        }
    }
    return 0;
}