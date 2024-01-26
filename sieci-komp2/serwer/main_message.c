#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#define PORT 1234
#define MAX_SIZE_PER_ROOM 2
#define BUF_SIZE 4096

unsigned int cCount = 0;
//int uid = 10;

void childend(int signo) {wait(NULL);}

struct Cln{
    int cfd;
    struct sockaddr_in caddr;
};struct Cln *clnArray[MAX_SIZE_PER_ROOM] = {};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void answer(void* buf, int rc){
    pthread_mutex_lock(&mutex);
    
    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArray[i])
            continue;

        int wr = 0;
        while(wr < rc){
            wr += write(clnArray[i]->cfd, buf + wr, rc - wr);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void addNewClient(struct Cln *cln){
    pthread_mutex_lock(&mutex);

    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){  
        if(clnArray[i])
            continue;

        //clnArray[i] = malloc(sizeof(struct Cln));
        clnArray[i] = cln;
        //char buf[2];
        //sprintf(buf, "%d", i);
        //write(cln->cfd, buf, sizeof(buf));
        break;
    }
    
    pthread_mutex_unlock(&mutex);
}

void removeClient(struct Cln *cln){
    pthread_mutex_lock(&mutex);

    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArray[i] || clnArray[i] != cln)
            continue;

        clnArray[i] = NULL;
        break;
    }
    
    pthread_mutex_unlock(&mutex);
}

int connectionCheck(struct Cln *cln, void *buf, char *replyMessage, int replySize, int bytesAviable, int retryCount){
    int i = 1, rc = 0;
    char *testMessage;

    for(i; i <= replySize; i++){
        do{
            rc += read(cln->cfd, buf + rc, replySize * i - rc);
        }while(rc < replySize * i);

        memcpy(testMessage, buf, replySize);

        if(bytesAviable - replySize * i < replySize * (i+1) || strcmp(testMessage, replyMessage) != 0){
            break;
        }
        rc = 0;
    }

    return rc;
}

void *cHandle(void *arg){
    struct Cln *cln = (struct Cln*) arg;

    void *buf = malloc(BUF_SIZE);
    char *conncectionTestMessage = "test", end;
    int rc, bytesAviable, retryCount, replySize = strlen(conncectionTestMessage);

    addNewClient(cln);
    cCount++;

    while(1){
        retryCount = 0;
        do{
            retryCount++;
            write(cln->cfd, conncectionTestMessage, replySize);
            sleep(2);
            ioctl(cln->cfd, FIONREAD, &bytesAviable);
        }while(bytesAviable < replySize && retryCount < 40);

        if(bytesAviable < replySize){
            removeClient(cln);
            break;
        }
        
        rc = connectionCheck(cln, buf, conncectionTestMessage, replySize, bytesAviable, retryCount);
        if(rc == 0) {
            //informacja zwrotna od klienta, bez wiadomosci
            continue;
        }

        answer(buf, rc);
        while(end != '\n' || rc <= 0){
            rc += read(cln->cfd, buf + rc, BUF_SIZE - rc);
            //write(1, buf, sizeof(buf));
            answer(buf, rc);
            end = ((char*)buf)[rc-1];
        }
        
        
    }close(cln->cfd);
    free(cln);
    cCount--;
    pthread_detach(pthread_self());
}

int main(int argc, char** argv){
    pthread_t pThread;
    int sfd, cfd, on = 1;
    struct sockaddr_in saddr, caddr;

    signal(SIGCHLD, childend);
    setsockopt(cfd,SOL_SOCKET,SO_REUSEADDR, (int*) 1,sizeof(int));
    
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORT);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
    bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
    listen(sfd, MAX_SIZE_PER_ROOM);

    char* error = "limit reached";

    while(1){
        socklen_t cl = sizeof(caddr); //client length  v
        cfd = accept(sfd, (struct sockaddr*) &caddr, &cl);

        if(cCount >= MAX_SIZE_PER_ROOM){
            
            int wr = 0;
            while(wr < strlen(error)){
                wr += write(cfd, error + wr, strlen(error) - wr);
            }
            close(cfd);
            continue;
        }
        printf("Nowe poloczenie %d\n", cfd);

        struct Cln *cln = (struct Cln*) malloc(sizeof(struct Cln));
        cln->cfd = cfd;
        cln->caddr = caddr;

        pthread_create(&pThread, NULL, &cHandle, (void*) cln);

    }close(sfd);
    free(clnArray);
    return EXIT_SUCCESS;
}