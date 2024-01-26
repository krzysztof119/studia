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
#include <sys/time.h>

//#define PROTOTYPE_CHECK_ACTIVITY 1

#define PORT 1234
#define MAX_SIZE_PER_ROOM 2
#define BUF_SIZE_TCP 32768
#define BUF_SIZE_UDP 2<<13
#define SECONDS_TO_WAIT 5
#define POSSIBLE_ATTEMPTS 4
#define END_PACKAGE_CHARACTER '\n'
#define SERVER_FREQUENCY 0.1

unsigned int cCount = 0;
//int uid = 10;

void childend(int signo) {wait(NULL);}

struct Cln{
    int cfd;
    struct sockaddr_in caddr;
};struct Cln *clnArray[MAX_SIZE_PER_ROOM] = {};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void answer(struct Cln *cln,void* buf, int rc){
    pthread_mutex_lock(&mutex);
    
    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArray[i] || clnArray[i] == cln)
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

        clnArray[i] = cln;
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

#if defined(PROTOTYPE_CHECK_ACTIVITY)
int connectionCheck(struct Cln *cln, void *buf, char *replyMessage, int replySize, int bytesAviable, int retryCount, int *bytesDeleted){
    int i = 1, rc = 0;
    char *testMessage;
    *bytesDeleted = 0;

    for(i; i <= replySize; i++){
        do{
            rc += read(cln->cfd, buf + rc, replySize - rc);
        }while(rc < replySize);
        *bytesDeleted += rc;

        memcpy(testMessage, buf, replySize);

        if(bytesAviable - replySize * i < replySize * (i+1) || strcmp(testMessage, replyMessage) != 0){
            break;
        }
        rc = 0;
    } *bytesDeleted -= rc;

    return rc;
}
#endif

void *cHandle(void *arg){
    struct Cln *cln = (struct Cln*) arg;
    void *buf = malloc(BUF_SIZE_TCP);
    
    char end;
    int rc;
    unsigned int bytesAviable;

#if defined(PROTOTYPE_CHECK_ACTIVITY)
    char *conncectionTestMessage = "test";
    int retryCount, replySize = strlen(conncectionTestMessage);
    unsigned int bytesDeleted;

    struct timeval beginTime, endTime;
    double elapsedTime;
#endif

    addNewClient(cln);
    cCount++;

    while(1){
#if defined(PROTOTYPE_CHECK_ACTIVITY)
        ettimeofday(&beginTime, 0);
        retryCount = 0;
#endif

        while(1){
            ioctl(cln->cfd, FIONREAD, &bytesAviable);

#if !defined(PROTOTYPE_CHECK_ACTIVITY)
            if(bytesAviable > 0){
                break;
            }
#endif

#if defined(PROTOTYPE_CHECK_ACTIVITY)
            if(bytesAviable > 0 || retryCount >= POSSIBLE_ATTEMPTS){
                break;
            }
            

            gettimeofday(&endTime, 0);
            elapsedTime = endTime.tv_sec - beginTime.tv_sec;

            
            if(elapsedTime >= SECONDS_TO_WAIT){
                retryCount++;
                write(cln->cfd, conncectionTestMessage, replySize);
            }
#endif
            sleep(SERVER_FREQUENCY);
        }

#if defined(PROTOTYPE_CHECK_ACTIVITY)
        if(bytesAviable == 0 && retryCount >= POSSIBLE_ATTEMPTS){
            removeClient(cln);
            break;
        }

        if(bytesAviable < replySize){
            continue;
        }
        
        if(retryCount == 0){
            rc = 0;
        } else {
            rc = connectionCheck(cln, buf, conncectionTestMessage, replySize, bytesAviable, retryCount, &bytesDeleted);
            if(rc == 0) { //informacja zwrotna od klienta, bez wiadomosci
                continue;
            }
        }
#endif

#if !defined(PROTOTYPE_CHECK_ACTIVITY)
        rc = 0;
#endif
        while(1){
            rc += read(cln->cfd, buf + rc, BUF_SIZE_TCP - rc);
            //write(1, buf, sizeof(buf));
            end = ((char*)buf)[rc-1];

#if !defined(PROTOTYPE_CHECK_ACTIVITY)
            if(rc == BUF_SIZE_TCP){
                answer(cln, buf, rc);
                rc = 0;
                bytesAviable -= rc;
            }

            if (rc >= bytesAviable){
                if(rc != BUF_SIZE_TCP){
                    answer(cln, buf, rc);
                }
                break;
            }
#endif
            
#if defined(PROTOTYPE_CHECK_ACTIVITY)
            if(rc == BUF_SIZE_TCP || rc == bytesAviable - bytesDeleted){
                answer(cln, buf, rc);
                rc = 0;
            }

            
            if (end == END_PACKAGE_CHARACTER || rc == bytesAviable - bytesDeleted)
            {
                break;
            }
#endif
        } 
    }
    close(cln->cfd); free(cln); free(buf);
    cCount--;
    pthread_detach(pthread_self());
}

void serverTCP(){
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
}

void serverUDP(){
    int fd, rc;
    socklen_t sl;
    void *buf = malloc(BUF_SIZE_TCP);
    struct sockaddr_in sa, ca;

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(1234);
    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(fd, (struct sockaddr*)&sa, sizeof(sa));
    while(1) {
    sl = sizeof(ca);
    rc = recvfrom(fd, buf, sizeof(buf), 0,
    (struct sockaddr*)&ca, &sl);
    write(1, buf, rc);
    
    sendto(fd, buf, rc, 0, (struct sockaddr*)&ca, sl);
    }
}

int main(int argc, char** argv){

        serverTCP();
    
    return EXIT_SUCCESS;
}
