    #include <netdb.h>
    #include <pthread.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
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

    #if !defined(PROTOTYPE_CHECK_ACTIVITY)
        #define SECONDS_TO_WAIT 600
    #endif

    #if defined(PROTOTYPE_CHECK_ACTIVITY)
        #define SECONDS_TO_WAIT_PROTOTYPE 20
        #define POSSIBLE_ATTEMPTS_PROTOTYPE 3
    #endif

    #define PORT 1234
    #define MAX_SIZE_PER_ROOM 100
    #define BUF_SIZE_TCP 32768
    #define BUF_SIZE_UDP 8192
    #define PACKET_TCP_SIZE 32768
    #define CONNECTION_TEST_MESSAGE (char*)"test"
    #define END_PACKAGE_CHARACTER '\n'
    #define SERVER_FREQUENCY 0.1
    #define CLIENT_NAME_SIZE 5
    #define SET_CLIENT_NAME_TYPE (unsigned char)'z'
    #define SERVER_LIST_REQUEST_TYPE (unsigned char)'x'
    #define TRANSFER_MESSAGE_TYPE (unsigned char)'c'
    #define RETURN_CLIENT_NAME_LIST (unsigned char*)"v"

    unsigned int cCount = 0;
    //int uid = 10;

    void childend(int signo) {wait(NULL);}

    struct ClnTCP{
        int cfd;
        unsigned char name[CLIENT_NAME_SIZE + 1]; // +1 - end of string
        struct sockaddr_in caddr;
        struct timeval beginTime;
    };struct ClnTCP *clnArrayTCP[MAX_SIZE_PER_ROOM] = {};
    pthread_mutex_t mutexTCP = PTHREAD_MUTEX_INITIALIZER;

    void answer(struct ClnTCP *clnTCP, void* buf, int rc){
        pthread_mutex_lock(&mutexTCP);
        
        for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
            if(!clnArrayTCP[i])//|| clnArrayTCP[i] == clnTCP
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

            char feedback[32]; sprintf(feedback, "odloczono poloczenie TCP: %d\n", clnTCP->cfd);
            (void) !write(1, feedback, strlen(feedback) * sizeof(*feedback));
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

    void sendClientList(struct ClnTCP *clnTCP, int wr){
        pthread_mutex_lock(&mutexTCP);
        unsigned int packageSize = (cCount-1) * CLIENT_NAME_SIZE;
            (void) !write(clnTCP->cfd, RETURN_CLIENT_NAME_LIST, 1);
            (void) !write(clnTCP->cfd, &packageSize, 4);
            
            for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
                if(!clnArrayTCP[i] || clnArrayTCP[i] == clnTCP){
                    continue;
                }
            (void) !write(clnTCP->cfd, clnArrayTCP[i]->name, CLIENT_NAME_SIZE);
            }
        pthread_mutex_unlock(&mutexTCP);
    }

    void messageHandler(struct ClnTCP *clnTCP, void *buf){
        int rc = 0, wr = 0;
        unsigned char type;
        rc = read(clnTCP->cfd, buf, 1);
        type = (unsigned char)((unsigned char*)buf)[0];

        switch (type)
        {
        case SET_CLIENT_NAME_TYPE:
            pthread_mutex_lock(&mutexTCP);
            //char *feedback;sprintf();
            (void) !write(1, (char*)"z\n", 2);

            while(1){
                rc += read(clnTCP->cfd, clnTCP->name, CLIENT_NAME_SIZE);
                
                if(rc == CLIENT_NAME_SIZE + 1){
                    break;
                }

            }clnTCP->name[CLIENT_NAME_SIZE] = '\0';
            pthread_mutex_unlock(&mutexTCP);

            sendClientList(clnTCP, wr);
            //(void) !write(clnTCP->cfd, clnTCP->name, CLIENT_NAME_SIZE);

            break;

        case SERVER_LIST_REQUEST_TYPE:
            (void) !write(1, (char*)"x\n", 2);
            sendClientList(clnTCP, wr);

            break;

        case TRANSFER_MESSAGE_TYPE: 
            (void) !write(1, (char*)"c\n", 2);
            unsigned char destination[CLIENT_NAME_SIZE + 1]; // +1 - end of string
            struct ClnTCP *receiver = NULL;

            while(1){
                
                rc += read(clnTCP->cfd, destination, CLIENT_NAME_SIZE); 
                
                if(rc == CLIENT_NAME_SIZE + 1){
                    break;
                }

            }destination[CLIENT_NAME_SIZE] = '\0';

            pthread_mutex_lock(&mutexTCP);
            for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
                if(strcmp(clnArrayTCP[i]->name, destination) == 0){
                    receiver = clnArrayTCP[i];
                    break;
                }
            }
            pthread_mutex_unlock(&mutexTCP);

            if(!receiver){
                (void) !write(1, (char*)"nie znaleziono klienta\n", 23);
                break;
            }
            (void) !write(1, (char*)"znaleziono klienta\n", 19);
            //strncpy(buf+1, destination, CLIENT_NAME_SIZE);

            pthread_mutex_lock(&mutexTCP);
            
            wr = 0;
            wr += write(receiver->cfd, &((unsigned char*)buf)[0], 1);
            while(1){
                wr += write(receiver->cfd, destination, CLIENT_NAME_SIZE);
            
                if(wr == CLIENT_NAME_SIZE + 1){
                    break;
                }
            }

            pthread_mutex_unlock(&mutexTCP);

            int tempBufSize = BUF_SIZE_TCP;
            while(1){
                if(rc == tempBufSize){
                    tempBufSize += BUF_SIZE_TCP;
                    if(tempBufSize > PACKET_TCP_SIZE){
                        tempBufSize = PACKET_TCP_SIZE;
                    }
                }

                rc += read(clnTCP->cfd, buf, tempBufSize - rc);

                pthread_mutex_lock(&mutexTCP);
                while(wr == rc){
                    wr += write(receiver->cfd, buf + (rc - wr), tempBufSize - wr);
                }
                pthread_mutex_unlock(&mutexTCP);
                
                if(rc == PACKET_TCP_SIZE){
                    break;
                }
            }
            break;

        default:
            break;
        }
    }

    void *cHandleTCP(void *arg){
        struct ClnTCP *clnTCP = (struct ClnTCP*) arg;
        void *buf = malloc(BUF_SIZE_TCP);
        
        char end;
        int rc;
        unsigned int bytesAviable;

    #if !defined(PROTOTYPE_CHECK_ACTIVITY)
        int disconnect = 0;

    #endif
    #if defined(PROTOTYPE_CHECK_ACTIVITY)
        int retryCount, replySize = strlen(CONNECTION_TEST_MESSAGE);
        unsigned int bytesDeleted;
    #endif

        struct timeval beginTime, endTime;
        double elapsedTime;

        addNewClient(clnTCP);
        cCount++;

        while(1){
            gettimeofday(&clnTCP->beginTime, 0);

    #if defined(PROTOTYPE_CHECK_ACTIVITY)
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
    #endif            

                gettimeofday(&endTime, 0);
                elapsedTime = endTime.tv_sec - clnTCP->beginTime.tv_sec;

    #if !defined(PROTOTYPE_CHECK_ACTIVITY)
                if(elapsedTime >= SECONDS_TO_WAIT){
                    disconnect = 1;
                    break;
                }
    #endif

    #if defined(PROTOTYPE_CHECK_ACTIVITY)
                if(elapsedTime >= SECONDS_TO_WAIT){
                    retryCount++;
                    (void) !write(clnTCP->cfd, CONNECTION_TEST_MESSAGE, replySize);
                }
    #endif
                sleep(SERVER_FREQUENCY);
            }

    #if !defined(PROTOTYPE_CHECK_ACTIVITY)
                if(disconnect){
                    removeClient(clnTCP);
                    break;
                }
    #endif

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
            messageHandler(clnTCP, buf);
            /*
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
            */
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

            (void) !printf("Nowe poloczenie TCP: %d\n", cfd);
            //fflush(stdout);

            struct ClnTCP *clnTCP = (struct ClnTCP*) malloc(sizeof(struct ClnTCP));
            clnTCP->cfd = cfd;
            clnTCP->caddr = caddr;

            pthread_create(&pThread, NULL, &cHandleTCP, (void*) clnTCP);

        }close(sfd);
        free(clnArrayTCP);
    }

    void serverUDP(){
        int fd, rc;
        socklen_t sl;
        void *buf = malloc(BUF_SIZE_TCP);

        struct sockaddr_in sa, ca;

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        sa.sin_port = htons(1235);
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(fd, (struct sockaddr*)&sa, sizeof(sa));
        while(1) {
            int notAllowed = 1;
            sl = sizeof(ca);

            rc = recvfrom(fd, buf, BUF_SIZE_UDP, 0, (struct sockaddr*)&ca, &sl);
            if(rc <= 0){
                continue;
            }
            
            for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
                if(!clnArrayTCP[i] || !clnArrayTCP[i]->caddr.sin_addr.s_addr == ca.sin_addr.s_addr){//|| clnArrayTCP[i] == clnTCP
                    continue;
                }
                gettimeofday(&clnArrayTCP[i]->beginTime, 0);
                notAllowed = 0;
            }

            if(notAllowed){
                continue;
            }
            
            (void) !printf("Nowe poloczenie UDP: %d\n", fd);
            fflush(stdout);

            //(void) !write(1, buf, rc);

            for(int i = 0; i < MAX_SIZE_PER_ROOM; i++){
                if(!clnArrayTCP[i] || clnArrayTCP[i]->caddr.sin_addr.s_addr == ca.sin_addr.s_addr)//|| clnArrayTCP[i] == clnTCP
                    continue;

                int wr = 0;
                while(wr < rc){
                    wr += sendto(fd, buf, rc, 0, (struct sockaddr*)&clnArrayTCP[i]->caddr, sizeof(clnArrayTCP[i]->caddr));
                }
            }
            
        }close(fd);
    }

    int main(int argc, char** argv){
        serverTCP();
        
        return EXIT_SUCCESS;
    }
