#include "process.h"
#include <stdio.h>
#define BSIZE 256

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int menu()
{
    fputs("1.stdin\n",stdout);
    fputs("2.plik\n",stdout);
    fputs("Wybierz opcje:",stdout);
    int opcja;
    scanf("%d",&opcja);
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


int process1(const char *fifo1Loc)
{
    char line[BSIZE];
    int option, run=1;
    while(run)
    {
        option = menu();
        int fifo1FD1;
        switch(option)
        {
            
            case 1:
                fflush(stdin);
                fifo1FD1 = open(fifo1Loc,O_WRONLY);
                while(line[0]!='.'){
                    fgets(line,BSIZE,stdin);
                    if(line[0]!='.' && write(fifo1FD1,line,BSIZE) == -1){
                        printf("blad przy zapisie buffera do fifo\n");
                        return 3; 
                    }
                }
                for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                write(fifo1FD1,line,BSIZE);
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
                        return 5;
                    }
                    fifo1FD1= open(fifo1Loc,O_WRONLY);
                    int readPossible=1;
                    while(readPossible)
                    {
                        
                        for(int i=0;i<BSIZE;i++) line[i]=0; //buffer clr
                        readPossible = readLine(fileIn,line);
                        //printf("read=%d line: %s \n",readPossible,line);
                        write(fifo1FD1,line,BSIZE);
                        
                    }
                    fclose(fileIn);
                    close(fifo1FD1);
                } 
        break;
        default:
        printf("Error - niewlasciwa opcja\n");   
        }
    }
    return 0; 
}

int process2(const char*fifo1Loc,const char *fifo2Loc)
{//odbiera od 0 i liczy il znakow
    int run=1;
    while(run)
    {
        int fifo1FD2 = open(fifo1Loc,O_RDONLY);
        int fifo2FD1 = open(fifo2Loc,O_WRONLY);
        char rdBuffer[BSIZE];
        int charNum=1;
        while(charNum>0) //wlicza \n
        {
            charNum=0;
            read(fifo1FD2,rdBuffer,BSIZE);
            while(rdBuffer[charNum]!=0 && rdBuffer[charNum++]!='\n');
            //printf("Cnum: %d\n",charNum);
            write(fifo2FD1,&charNum,sizeof(int));    
        }
        close(fifo1FD2);
        close(fifo2FD1);
    }
    return 0;
}

void p3_handler(int signal)
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
    //kill(getppid(),signal);
}


int process3(const char *fifo2Loc)
{  
    int run=1;
    struct sigaction sa_term;
    sa_term.sa_handler = p3_handler;
    sa_term.sa_flags=SA_RESTART;
    sigaction(SIGTERM,&sa_term,NULL);
    sigaction(SIGTSTP,&sa_term,NULL);
    sigaction(SIGCONT,&sa_term,NULL);
    while(run)
    {
        int fifo2FD2 = open(fifo2Loc,O_RDONLY);
        int charRecv=1;
        while(charRecv>0)
        {
            read(fifo2FD2,&charRecv,sizeof(int));
            if(charRecv>0) printf("%d\n",charRecv-1);
        }
        close(fifo2FD2);
    }
    return 0;
}