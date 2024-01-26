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
#define BUF_SIZE_UDP 8192
#define SECONDS_TO_WAIT 5
#define POSSIBLE_ATTEMPTS 4
#define CONNECTION_TEST_MESSAGE (char*)"test"
#define END_PACKAGE_CHARACTER '\n'
#define SERVER_FREQUENCY 0.1

unsigned int cCount = 0;
//int uid = 10;

void childend(int signo) {wait(NULL);}

struct ClnTCP{
    int cfd;
    struct sockaddr_in caddr;
};struct ClnTCP *clnArrayTCP[MAX_SIZE_PER_ROOM] = {};
pthread_mutex_t mutexTCP = PTHREAD_MUTEX_INITIALIZER;

struct ClnUDP{
    int fd;
    socklen_t sl;
    struct sockaddr_in ca;
};
pthread_mutex_t mutexUDP = PTHREAD_MUTEX_INITIALIZER;

void answer(struct ClnTCP *clnTCP,void* buf, int rc){
    pthread_mutex_lock(&mutexTCP);
    
    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArrayTCP[i] || clnArrayTCP[i] == clnTCP)
            continue;

        int wr = 0;
        while(wr < rc){
            wr += write(clnArrayTCP[i]->cfd, buf + wr, rc - wr);
        }
    }

    pthread_mutex_unlock(&mutexTCP);
}

void addNewClient(struct ClnTCP *clnTCP){
    pthread_mutex_lock(&mutexTCP);

    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){  
        if(clnArrayTCP[i])
            continue;

        clnArrayTCP[i] = clnTCP;
        break;
    }
    
    pthread_mutex_unlock(&mutexTCP);
}

void removeClient(struct ClnTCP *clnTCP){
    pthread_mutex_lock(&mutexTCP);

    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArrayTCP[i] || clnArrayTCP[i] != clnTCP)
            continue;

        clnArrayTCP[i] = NULL;
        break;
    }
    
    pthread_mutex_unlock(&mutexTCP);
}

#if defined(PROTOTYPE_CHECK_ACTIVITY)
int connectionCheck(struct ClnTCP *clnTCP, void *buf, int replySize, int bytesAviable, int retryCount, int *bytesDeleted){
    int i = 1, rc = 0;
    char *testMessage;
    *bytesDeleted = 0;

    for(i; i <= replySize; i++){
        do{
            rc += read(clnTCP->cfd, buf + rc, replySize - rc);
        }while(rc < replySize);
        *bytesDeleted += rc;

        memcpy(testMessage, buf, replySize);

        if(bytesAviable - replySize * i < replySize * (i+1) || strcmp(testMessage, CONNECTION_TEST_MESSAGE) != 0){
            break;
        }
        rc = 0;
    } *bytesDeleted -= rc;

    return rc;
}
#endif

void *cHandleTCP(void *arg){
    struct ClnTCP *clnTCP = (struct ClnTCP*) arg;
    void *buf = malloc(BUF_SIZE_TCP);
    
    char end;
    int rc;
    unsigned int bytesAviable;

#if defined(PROTOTYPE_CHECK_ACTIVITY)
    int retryCount, replySize = strlen(CONNECTION_TEST_MESSAGE);
    unsigned int bytesDeleted;

    struct timeval beginTime, endTime;
    double elapsedTime;
#endif

    addNewClient(clnTCP);
    cCount++;

    while(1){
#if defined(PROTOTYPE_CHECK_ACTIVITY)
        ettimeofday(&beginTime, 0);
        retryCount = 0;
#endif

        while(1){
            ioctl(clnTCP->cfd, FIONREAD, &bytesAviable);

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
                (void) !write(clnTCP->cfd, CONNECTION_TEST_MESSAGE, replySize);
            }
#endif
            sleep(SERVER_FREQUENCY);
        }

#if defined(PROTOTYPE_CHECK_ACTIVITY)
        if(bytesAviable == 0 && retryCount >= POSSIBLE_ATTEMPTS){
            removeClient(clnTCP);
            break;
        }

        if(bytesAviable < replySize){
            continue;
        }
        
        if(retryCount == 0){
            rc = 0;
        } else {
            rc = connectionCheck(clnTCP, buf, replySize, bytesAviable, retryCount, &bytesDeleted);
            if(rc == 0) { //informacja zwrotna od klienta, bez wiadomosci
                continue;
            }
        }
#endif

#if !defined(PROTOTYPE_CHECK_ACTIVITY)
        rc = 0;
#endif
        while(1){
            rc += read(clnTCP->cfd, buf + rc, BUF_SIZE_TCP - rc);
            //(void) !write(1, buf, sizeof(buf));
            end = ((char*)buf)[rc-1];

#if !defined(PROTOTYPE_CHECK_ACTIVITY)
            if(rc == BUF_SIZE_TCP){
                answer(clnTCP, buf, rc);
                break;
            }

            if (rc >= bytesAviable || end == END_PACKAGE_CHARACTER){    
                if(rc != BUF_SIZE_TCP){
                    answer(clnTCP, buf, rc);
                }
                break;
            }
#endif
            
#if defined(PROTOTYPE_CHECK_ACTIVITY)
            if(rc == BUF_SIZE_TCP || rc == bytesAviable - bytesDeleted){
                answer(clnTCP, buf, rc);
                rc = 0;
            }

            
            if (end == END_PACKAGE_CHARACTER || rc == bytesAviable - bytesDeleted)
            {
                break;
            }
#endif
        } 
    }
    close(clnTCP->cfd); free(clnTCP); free(buf);
    cCount--;
    pthread_detach(pthread_self());
}

void serverTCP(){
    pthread_t pThread;
    int sfd, cfd, on = 1;
    struct sockaddr_in saddr, caddr;

    signal(SIGCHLD, childend);
    //setsockopt(cfd,SOL_SOCKET,SO_REUSEADDR, (int*) 1,sizeof(int));
    
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

        struct ClnTCP *clnTCP = (struct ClnTCP*) malloc(sizeof(struct ClnTCP));
        clnTCP->cfd = cfd;
        clnTCP->caddr = caddr;

        pthread_create(&pThread, NULL, &cHandleTCP, (void*) clnTCP);

    }close(sfd);
    free(clnArrayTCP);
}

void *cHandleUDP(void *arg){
    struct ClnUDP *clnUDP = (struct ClnUDP*) arg;
    int rc;
    void *buf = malloc(BUF_SIZE_TCP);

    rc = recvfrom(clnUDP->fd, buf, sizeof(buf), 0, (struct sockaddr*)&clnUDP->ca, &clnUDP->sl);
    //(void) !write(1, buf, rc);
    pthread_mutex_lock(&mutexUDP);

    for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
        if(!clnArrayTCP[i]) // && clnArrayTCP[i]->caddr.sin_port == clnUDP->ca.sin_port
            continue;

        int wr = 0;
        while(wr < rc){
            sendto(clnUDP->fd, buf, rc, 0, (struct sockaddr*)&clnUDP->ca, clnUDP->sl);
        }
    }
    
    pthread_mutex_lock(&mutexUDP);
}

void serverUDP(){
    pthread_t pThread;
    int fd;
    socklen_t sl;

    signal(SIGCHLD, childend);

    struct sockaddr_in sa, ca;

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(1235);
    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(fd, (struct sockaddr*)&sa, sizeof(sa));
    while(1) {
        sl = sizeof(ca);

        //printf("Nowe poloczenie %d\n", fd);

        struct ClnUDP *clnUDP = (struct ClnUDP*) malloc(sizeof(struct ClnUDP));
        clnUDP->fd = fd;
        clnUDP->sl = sl;
        clnUDP->ca = ca;

        pthread_create(&pThread, NULL, &cHandleUDP, (void*) clnUDP);
    
    }
}

int main(int argc, char** argv){
    if(fork() == 0){
        serverTCP();
    } else{
        serverUDP();
    }
    
    return EXIT_SUCCESS;
}
